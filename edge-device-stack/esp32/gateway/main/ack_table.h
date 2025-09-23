#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ACK_TABLE_MAX 32

typedef struct {
    char correlation_id[40];
    uint32_t last_attempt_ms;
    uint8_t attempt;
    bool in_use;
} ack_entry_t;

void ack_table_init(void);
void ack_table_track(const char *correlation_id);
bool ack_table_should_retry(const char *correlation_id, uint32_t timeout_ms, uint8_t max_retries);
void ack_table_clear(const char *correlation_id);
void ack_table_snapshot(char *buffer, size_t length);

#ifdef __cplusplus
}
#endif
