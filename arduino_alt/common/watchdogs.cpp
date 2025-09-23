#include "watchdogs.h"

#include <sdkconfig.h>
#include <esp_err.h>
#include <esp_idf_version.h>
#include <esp_task_wdt.h>

namespace watchdogs {

namespace {
TaskHandle_t comm_task = nullptr;
TaskHandle_t sensor_task = nullptr;
uint32_t timeout_ms_global = 8000;
}

void begin(uint32_t timeout_ms) {
    if (timeout_ms == 0) {
        timeout_ms = 8000;
    }
    timeout_ms_global = timeout_ms;
    esp_task_wdt_config_t config = {};
    config.timeout_ms = timeout_ms_global;
    config.trigger_panic = true;
#ifdef CONFIG_FREERTOS_UNICORE
    config.idle_core_mask = 0x1;
#else
    config.idle_core_mask = (1 << portNUM_PROCESSORS) - 1;
#endif
    esp_err_t err = esp_task_wdt_init(&config);
    if (err == ESP_ERR_INVALID_STATE) {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
        esp_task_wdt_reconfigure(&config);
#else
        esp_task_wdt_deinit();
        esp_task_wdt_init(&config);
#endif
    }
    esp_task_wdt_add(nullptr);
}

void register_comm_task(TaskHandle_t handle) {
    comm_task = handle;
    if (comm_task) {
        esp_task_wdt_add(comm_task);
    }
}

void register_sensor_task(TaskHandle_t handle) {
    sensor_task = handle;
    if (sensor_task) {
        esp_task_wdt_add(sensor_task);
    }
}

void feed_comm() {
    esp_task_wdt_reset();
}

void feed_sensor() {
    esp_task_wdt_reset();
}

void feed_main() {
    esp_task_wdt_reset();
}

}  // namespace watchdogs

