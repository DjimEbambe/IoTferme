#include "light_adc.h"

#include "driver/adc.h"
#include "esp_check.h"
#include "esp_adc_cal.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pins.h"

static const char *TAG = "light";
static esp_adc_cal_characteristics_t s_chars;
static bool s_ready = false;

esp_err_t light_sensor_init(void)
{
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(EDGE_LIGHT_ADC_CHANNEL, ADC_ATTEN_DB_11);
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &s_chars);
    s_ready = true;
    ESP_LOGI(TAG, "init channel %d", EDGE_LIGHT_ADC_CHANNEL);
    return ESP_OK;
}

esp_err_t light_sensor_read(light_sample_t *sample)
{
    if (!s_ready) {
        ESP_RETURN_ON_ERROR(light_sensor_init(), TAG, "init");
    }
    uint32_t accum = 0;
    const int samples = 32;
    for (int i = 0; i < samples; ++i) {
        accum += adc1_get_raw(EDGE_LIGHT_ADC_CHANNEL);
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    uint32_t avg = accum / samples;
    uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(avg, &s_chars);
    sample->lux = (float)voltage_mv * 0.5f;  // placeholder linear approximation
    return ESP_OK;
}
