#pragma once

#include <Arduino.h>

namespace time_sync {

void begin();
void update_from_epoch_ms(uint64_t epoch_ms);
uint64_t now_epoch_ms();
String now_iso();
int64_t offset_ms();

}  // namespace time_sync

