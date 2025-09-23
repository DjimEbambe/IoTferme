#include <Arduino.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <new>
#include <string.h>

#include "../common/board_detect.h"
#include "../common/cmd_exec.h"
#include "../common/config.h"
#include "../common/debug_log.h"
#include "../common/espnow_link.h"
#include "../common/opts.h"
#include "../common/payload.h"
#include "../common/pins_tlorac6.h"
#include "../common/pins_trelay_s3.h"
#include "../common/pzem_modbus.h"
#include "../common/ringbuf_fs.h"
#include "../common/relays.h"
#include "../common/time_sync.h"
#include "../common/watchdogs.h"

using namespace config;
#include "../common/board_detect.cpp"
#include "../common/debug_log.cpp"
#include "../common/espnow_link.cpp"
#include "../common/payload.cpp"
#include "../common/relays.cpp"
#include "../common/ringbuf_fs.cpp"
#include "../common/pzem_modbus.cpp"
#include "../common/time_sync.cpp"
#include "../common/watchdogs.cpp"

namespace {

struct TelemetryMessage {
    size_t length = 0;
    uint8_t payload[config::kEspNowPayloadMax];
    bool urgent = false;
    bool persistent = true;
    String category;
    String timestamp;
};

TaskHandle_t task_measure = nullptr;
TaskHandle_t task_comm = nullptr;
QueueHandle_t q_tx = nullptr;
QueueHandle_t q_rx = nullptr;

board_detect::BoardType g_board = board_detect::BoardType::kUnknown;
const DeviceIdentity *g_identity = &config::kPzemIdentity;

relays::RelayBank g_relays;
storage::RingbufFS g_storage;
cmd_exec::Setpoints g_setpoints;
sensors::PzemModbus g_pzem;
espnow_link::EspNowLink &g_link = espnow_link::instance();

float g_last_voltage = 0.0f;
float g_last_current = 0.0f;
float g_last_power = 0.0f;
float g_last_energy = 0.0f;
float g_last_pf = 1.0f;
float g_last_freq = 50.0f;

struct DevicePins {
    uint8_t relay_pump;
    int8_t rs485_re;
    int8_t rs485_rx;
    int8_t rs485_tx;
};

DevicePins g_pins;

void queue_message(const TelemetryMessage &msg, bool persist = true) {
    TelemetryMessage *copy = new (std::nothrow) TelemetryMessage(msg);
    if (!copy) {
        LOGW("queue alloc failed");
        return;
    }
    copy->persistent = persist;
    if (xQueueSend(q_tx, &copy, portMAX_DELAY) != pdTRUE) {
        delete copy;
        LOGW("tx queue full");
        return;
    }
    if (persist) {
        g_storage.append(String(msg.category), msg.payload, msg.length, String(msg.timestamp));
    }
}

void fill_pins() {
    if (board_detect::is_s3(g_board)) {
        g_pins.relay_pump = pins_trelay_s3::RELAY_AUX1;
        g_pins.rs485_re = pins_trelay_s3::UART485_RE;
        g_pins.rs485_rx = pins_trelay_s3::UART485_RX;
        g_pins.rs485_tx = pins_trelay_s3::UART485_TX;
    } else {
        g_pins.relay_pump = pins_tlorac6::RELAY_PUMP;
        g_pins.rs485_re = pins_tlorac6::UART485_RE;
        g_pins.rs485_rx = pins_tlorac6::UART485_RX;
        g_pins.rs485_tx = pins_tlorac6::UART485_TX;
    }
}

void ensure_storage() {
    g_storage.begin(RINGBUF_TARGET_CAPACITY_BYTES);
    g_storage.purge_expired(RINGBUF_RETENTION_DAYS);
}

void enqueue_power(const sensors::PzemReadings &readings) {
    payload::TelemetryPower telemetry;
    telemetry.asset_id = g_identity->asset_id;
    telemetry.device = g_identity->device_id;
    telemetry.site = config::kSiteId;
    telemetry.timestamp_iso = time_sync::now_iso();
    telemetry.voltage = readings.voltage_v;
    telemetry.current = readings.current_a;
    telemetry.power_w = readings.power_w;
    telemetry.energy_Wh = readings.energy_Wh;
    telemetry.pf = readings.pf;
    telemetry.frequency = readings.frequency;
    telemetry.rssi_dbm = 0;
    telemetry.idempotency_key = payload::make_uuid();

    TelemetryMessage msg;
    msg.length = payload::encode_telemetry_power(telemetry, msg.payload, sizeof(msg.payload));
    msg.category = "telemetry_power";
    msg.timestamp = telemetry.timestamp_iso;
    queue_message(msg);
}

void enqueue_water(bool pump_on) {
    payload::TelemetryWater telemetry;
    telemetry.asset_id = g_identity->asset_id;
    telemetry.device = g_identity->device_id;
    telemetry.site = config::kSiteId;
    telemetry.timestamp_iso = time_sync::now_iso();
    telemetry.level = NAN;
    telemetry.flow_l_min = NAN;
    telemetry.pump_on = pump_on;
    telemetry.idempotency_key = payload::make_uuid();

    TelemetryMessage msg;
    msg.length = payload::encode_telemetry_water(telemetry, msg.payload, sizeof(msg.payload));
    msg.category = "telemetry_water";
    msg.timestamp = telemetry.timestamp_iso;
    queue_message(msg);
}

void process_rx_frame(const espnow_link::Frame &frame) {
    payload::CommandMessage cmd;
    if (payload::decode_command(frame.payload, frame.length, cmd)) {
        payload::AckPayload ack;
        bool ok = true;
        if (cmd.pump_on_request) {
            ok &= g_relays.command(relays::kLamp, true);
        } else {
            ok &= g_relays.command(relays::kLamp, false);
        }
        if (cmd.reset_energy) {
            ok &= g_pzem.reset_energy();
        }
        ack.asset_id = g_identity->asset_id;
        ack.correlation_id = cmd.correlation_id;
        ack.ok = ok;
        ack.message = ok ? F("applied") : F("failed");
        ack.timestamp_iso = time_sync::now_iso();

        TelemetryMessage msg;
        msg.length = payload::encode_ack(ack, msg.payload, sizeof(msg.payload));
        msg.category = "ack";
        msg.timestamp = ack.timestamp_iso;
        msg.urgent = true;
        queue_message(msg, false);
        return;
    }
    StaticJsonDocument<512> doc;
#if USE_MSGPACK
    DeserializationError err = deserializeMsgPack(doc, frame.payload, frame.length);
#else
    DeserializationError err = deserializeJson(doc, frame.payload, frame.length);
#endif
    if (!err && doc.containsKey("act")) {
        const char *act = doc["act"].as<const char*>();
        if (act && strcmp(act, "timesync") == 0) {
            uint64_t epoch_ms = doc["epoch_ms"].as<uint64_t>();
            time_sync::update_from_epoch_ms(epoch_ms);
        }
    }
}

bool send_via_espnow(const TelemetryMessage &msg) {
    uint8_t mac[6];
    if (!g_link.resolve_mac_for_asset(config::kGatewayIdentity.asset_id, mac)) {
        return false;
    }
    return g_link.send(mac, msg.payload, msg.length);
}

void on_esp_frame(const espnow_link::Frame &frame) {
    if (!q_rx) {
        return;
    }
    BaseType_t woken = pdFALSE;
    xQueueSendFromISR(q_rx, &frame, &woken);
    if (woken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

void task_measure_entry(void *) {
    watchdogs::register_sensor_task(xTaskGetCurrentTaskHandle());
    uint32_t next_power = 0;
    uint32_t next_water = 0;
    for (;;) {
        watchdogs::feed_sensor();
        uint32_t now = millis();
        if (now >= next_power) {
            sensors::PzemReadings readings;
            if (g_pzem.read(readings)) {
                g_last_voltage = readings.voltage_v;
                g_last_current = readings.current_a;
                g_last_power = readings.power_w;
                g_last_energy = readings.energy_Wh;
                g_last_pf = readings.pf;
                g_last_freq = readings.frequency;
                enqueue_power(readings);
            }
            next_power = now + TELEMETRY_POWER_PERIOD_MS;
        }
        if (now >= next_water) {
            enqueue_water(g_relays.get(relays::kLamp));
            next_water = now + TELEMETRY_WATER_PERIOD_MS;
        }
        g_relays.loop();
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void task_comm_entry(void *) {
    watchdogs::register_comm_task(xTaskGetCurrentTaskHandle());
    for (;;) {
        watchdogs::feed_comm();
        g_link.loop();
        espnow_link::Frame frame;
        if (xQueueReceive(q_rx, &frame, 0) == pdTRUE) {
            process_rx_frame(frame);
        }
        TelemetryMessage *msg_ptr = nullptr;
        if (xQueueReceive(q_tx, &msg_ptr, 0) == pdTRUE && msg_ptr) {
            if (!send_via_espnow(*msg_ptr)) {
                vTaskDelay(pdMS_TO_TICKS(50));
                xQueueSendToFront(q_tx, &msg_ptr, 0);
                msg_ptr = nullptr;
            } else {
                delete msg_ptr;
                msg_ptr = nullptr;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void handle_cli_line(const String &line) {
    if (line.startsWith("pump on")) {
        g_relays.command(relays::kLamp, true);
    } else if (line.startsWith("pump off")) {
        g_relays.command(relays::kLamp, false);
    } else if (line.startsWith("reset energy")) {
        if (g_pzem.reset_energy()) {
            LOGI("Energy reset");
        }
    }
}

void handle_cli() {
    static String buffer;
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (buffer.length()) {
                handle_cli_line(buffer);
                buffer.clear();
            }
        } else {
            buffer += c;
        }
    }
}

}  // namespace

void setup() {
    g_board = board_detect::detect();
    fill_pins();

    debug_log::begin();
    LOGI("Device PZEM boot %s", board_detect::to_string(g_board));

    watchdogs::begin();
    time_sync::begin();

    uint8_t relay_map[relays::kRelayCount];
    for (size_t i = 0; i < relays::kRelayCount; ++i) {
        relay_map[i] = 0xFF;
    }
    relay_map[relays::kLamp] = g_pins.relay_pump;
    g_relays.begin(relay_map, relays::kRelayCount);
    ensure_storage();

    g_setpoints.t_c = NAN;
    g_setpoints.rh = NAN;
    g_setpoints.lux = NAN;

    g_pzem.begin(Serial1, 0x01, 9600, g_pins.rs485_re, g_pins.rs485_rx, g_pins.rs485_tx);

    q_tx = xQueueCreate(16, sizeof(TelemetryMessage *));
    q_rx = xQueueCreate(8, sizeof(espnow_link::Frame));

    g_link.set_receive_handler(on_esp_frame);
    g_link.begin();

    BaseType_t core_measure = 0;
    BaseType_t core_comm = board_detect::is_s3(g_board) ? 1 : 0;

    xTaskCreatePinnedToCore(task_measure_entry, "measure", 6144, nullptr, 2, &task_measure, core_measure);
    xTaskCreatePinnedToCore(task_comm_entry, "comm", 6144, nullptr, 3, &task_comm, core_comm);
}

void loop() {
    watchdogs::feed_main();
    handle_cli();
    vTaskDelay(pdMS_TO_TICKS(25));
}

