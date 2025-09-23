#include "ack_table.h"

#include <stdio.h>
#include <string.h>

#include "esp_system.h"
#include "esp_timer.h"

static ack_entry_t s_entries[ACK_TABLE_MAX];

static uint32_t now_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

void ack_table_init(void)
{
    memset(s_entries, 0, sizeof(s_entries));
}

void ack_table_track(const char *correlation_id)
{
    for (size_t i = 0; i < ACK_TABLE_MAX; ++i) {
        if (s_entries[i].in_use && strncmp(s_entries[i].correlation_id, correlation_id, sizeof(s_entries[i].correlation_id)) == 0) {
            s_entries[i].last_attempt_ms = now_ms();
            s_entries[i].attempt++;
            return;
        }
    }
    for (size_t i = 0; i < ACK_TABLE_MAX; ++i) {
        if (!s_entries[i].in_use) {
            s_entries[i].in_use = true;
            strlcpy(s_entries[i].correlation_id, correlation_id, sizeof(s_entries[i].correlation_id));
            s_entries[i].last_attempt_ms = now_ms();
            s_entries[i].attempt = 1;
            return;
        }
    }
}

bool ack_table_should_retry(const char *correlation_id, uint32_t timeout_ms, uint8_t max_retries)
{
    for (size_t i = 0; i < ACK_TABLE_MAX; ++i) {
        if (s_entries[i].in_use && strncmp(s_entries[i].correlation_id, correlation_id, sizeof(s_entries[i].correlation_id)) == 0) {
            uint32_t elapsed = now_ms() - s_entries[i].last_attempt_ms;
            if (elapsed >= timeout_ms && s_entries[i].attempt <= max_retries) {
                s_entries[i].attempt++;
                s_entries[i].last_attempt_ms = now_ms();
                return true;
            }
            return false;
        }
    }
    return false;
}

void ack_table_clear(const char *correlation_id)
{
    for (size_t i = 0; i < ACK_TABLE_MAX; ++i) {
        if (s_entries[i].in_use && strncmp(s_entries[i].correlation_id, correlation_id, sizeof(s_entries[i].correlation_id)) == 0) {
            memset(&s_entries[i], 0, sizeof(ack_entry_t));
            return;
        }
    }
}

void ack_table_snapshot(char *buffer, size_t length)
{
    int written = snprintf(buffer, length, "[");
    for (size_t i = 0; i < ACK_TABLE_MAX && written < (int)length; ++i) {
        if (!s_entries[i].in_use) {
            continue;
        }
        written += snprintf(
            buffer + written,
            length - written,
            "{\"corr\":\"%s\",\"attempt\":%u},",
            s_entries[i].correlation_id,
            s_entries[i].attempt
        );
    }
    if (written > 1 && buffer[written - 1] == ',') {
        buffer[written - 1] = ']';
    } else if (written < (int)length) {
        buffer[written++] = ']';
    }
    if (written < (int)length) {
        buffer[written] = '\0';
    } else {
        buffer[length - 1] = '\0';
    }
}
