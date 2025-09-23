#pragma once

/**
 * @file config.h
 * @brief Common compile-time configuration for gateway and devices.
 */

#define EDGE_SITE_ID            "KIN-GOLIATH"
#define EDGE_GATEWAY_ID         "esp32gw-01"
#define EDGE_DEVICE_ID          "esp32c6-01"
#define EDGE_DEVICE_ASSET_ENV   "A-PP-01"
#define EDGE_DEVICE_ASSET_POWER "A-PP-01"
#define EDGE_TELEMETRY_INTERVAL_NORMAL_MS   (10 * 1000)
#define EDGE_TELEMETRY_INTERVAL_FAST_MS     (2 * 1000)
#define EDGE_POWER_INTERVAL_MS              (5 * 1000)
#define EDGE_WATER_INTERVAL_MS              (10 * 1000)
#define EDGE_INCUBATOR_INTERVAL_MS          (5 * 1000)

#define EDGE_ESPNOW_CHANNEL     6
#define EDGE_ESPNOW_PMK         "kin-goliath-pmk"
#define EDGE_ESPNOW_LMK         "kin-goliath-lmk"
#define EDGE_ESPNOW_MAX_PEERS   32

#define EDGE_PAIRING_WINDOW_DEFAULT_S  120

#define EDGE_USB_UART_TX_BUFFER (512)
#define EDGE_USB_UART_RX_BUFFER (512)

#define EDGE_CMD_TIMEOUT_MS  3000
#define EDGE_CMD_RETRIES     2

#define EDGE_MSG_ID_LENGTH   16

#define EDGE_LOCAL_FW_VERSION "1.0.0"
