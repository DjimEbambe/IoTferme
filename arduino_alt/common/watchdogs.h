#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace watchdogs {

void begin(uint32_t timeout_ms = 8000);
void register_comm_task(TaskHandle_t handle);
void register_sensor_task(TaskHandle_t handle);
void feed_comm();
void feed_sensor();
void feed_main();

}  // namespace watchdogs

