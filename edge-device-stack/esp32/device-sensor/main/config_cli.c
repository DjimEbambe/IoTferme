#include "config_cli.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mq135_adc.h"
#include "nvs_store.h"
#include "relays.h"

static const char *TAG = "cli";

static void handle_command(char *line)
{
    char *cmd = strtok(line, " \r\n");
    if (!cmd) {
        return;
    }
    if (strcmp(cmd, "help") == 0) {
        printf("Commands: help, mq135.r0 <value>, relay <ch> <on|off>\n");
    } else if (strcmp(cmd, "mq135.r0") == 0) {
        char *value = strtok(NULL, " \r\n");
        if (!value) {
            printf("Usage: mq135.r0 <value>\n");
            return;
        }
        float r0 = strtof(value, NULL);
        if (r0 > 1.0f && r0 < 100.0f) {
            mq135_init(r0);
            nvs_store_set_float("mq135_r0", r0);
            printf("mq135 r0 updated %.2f\n", r0);
        } else {
            printf("invalid r0\n");
        }
    } else if (strcmp(cmd, "relay") == 0) {
        char *ch = strtok(NULL, " \r\n");
        char *state = strtok(NULL, " \r\n");
        if (!ch || !state) {
            printf("Usage: relay <index> <on|off>\n");
            return;
        }
        int idx = atoi(ch);
        bool on = strcmp(state, "on") == 0 || strcmp(state, "ON") == 0;
        relays_set((relay_channel_t)idx, on);
        printf("relay %d -> %s\n", idx, on ? "ON" : "OFF");
    } else {
        printf("unknown command\n");
    }
}

static void cli_task(void *arg)
{
    (void)arg;
    const int uart_num = UART_NUM_0;
    uart_config_t cfg = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(uart_num, &cfg);
    uart_driver_install(uart_num, 1024, 0, 0, NULL, 0);
    printf("CLI ready. Type help.\n");
    char line[128];
    size_t idx = 0;
    while (true) {
        uint8_t byte;
        int read = uart_read_bytes(uart_num, &byte, 1, pdMS_TO_TICKS(100));
        if (read == 1) {
            if (byte == '\n' || byte == '\r') {
                line[idx] = '\0';
                handle_command(line);
                idx = 0;
            } else if (idx < sizeof(line) - 1) {
                line[idx++] = (char)byte;
            }
        }
    }
}

void config_cli_start(void)
{
    static bool started = false;
    if (started) {
        return;
    }
    started = true;
    xTaskCreate(cli_task, "cli", 4096, NULL, 2, NULL);
}
