#pragma once

#include "sdkconfig.h"
#include "driver/adc.h"
#include "driver/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_IDF_TARGET_ESP32S3
#define EDGE_I2C_SDA_GPIO          17
#define EDGE_I2C_SCL_GPIO          18
#define EDGE_LIGHT_ADC_CHANNEL     ADC_CHANNEL_0
#define EDGE_LIGHT_ADC_GPIO        4
#define EDGE_MQ135_ADC_CHANNEL     ADC_CHANNEL_3
#define EDGE_MQ135_ADC_GPIO        7
#define EDGE_RS485_UART_NUM        UART_NUM_1
#define EDGE_RS485_TX_GPIO         8
#define EDGE_RS485_RX_GPIO         9
#define EDGE_RS485_DE_GPIO         21
#define EDGE_RS485_RE_GPIO         20
#define EDGE_RELAY_GPIO_BASE       2
#elif CONFIG_IDF_TARGET_ESP32C6
#define EDGE_I2C_SDA_GPIO          5
#define EDGE_I2C_SCL_GPIO          4
#define EDGE_LIGHT_ADC_CHANNEL     ADC_CHANNEL_2
#define EDGE_LIGHT_ADC_GPIO        3
#define EDGE_MQ135_ADC_CHANNEL     ADC_CHANNEL_4
#define EDGE_MQ135_ADC_GPIO        1
#define EDGE_RS485_UART_NUM        UART_NUM_1
#define EDGE_RS485_TX_GPIO         19
#define EDGE_RS485_RX_GPIO         18
#define EDGE_RS485_DE_GPIO         20
#define EDGE_RS485_RE_GPIO         21
#define EDGE_RELAY_GPIO_BASE       8
#else
#define EDGE_I2C_SDA_GPIO          8
#define EDGE_I2C_SCL_GPIO          9
#define EDGE_LIGHT_ADC_CHANNEL     ADC_CHANNEL_6
#define EDGE_LIGHT_ADC_GPIO        34
#define EDGE_MQ135_ADC_CHANNEL     ADC_CHANNEL_7
#define EDGE_MQ135_ADC_GPIO        35
#define EDGE_RS485_UART_NUM        UART_NUM_1
#define EDGE_RS485_TX_GPIO         23
#define EDGE_RS485_RX_GPIO         22
#define EDGE_RS485_DE_GPIO         21
#define EDGE_RS485_RE_GPIO         19
#define EDGE_RELAY_GPIO_BASE       25
#endif

#define EDGE_RELAY_COUNT           6

#ifdef __cplusplus
}
#endif
