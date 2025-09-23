#include "mq135_adc.h"

#include <math.h>

#include "driver/adc.h"
#include "esp_check.h"
#include "esp_adc_cal.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_store.h"
#include "pins.h"

static const char *TAG = "mq135";
static esp_adc_cal_characteristics_t s_chars;
static bool s_ready = false;
static float s_r0 = 10.0f;

esp_err_t mq135_init(float r0_default)
{
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(EDGE_MQ135_ADC_CHANNEL, ADC_ATTEN_DB_11);
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &s_chars);
    s_ready = true;
    s_r0 = r0_default;
    float stored = 0;
    if (nvs_store_get_float("mq135_r0", &stored, r0_default) == ESP_OK) {
        s_r0 = stored;
    }
    ESP_LOGI(TAG, "init r0=%.2f", s_r0);
    return ESP_OK;
}

static float mq135_compute_ppm(float ratio)
{
    // Rough approximation curve for MQ-135 air quality sensor
    const float slope = -2.769f;
    const float intercept = 2.978f;  // log10(ross)
    float log_ppm = slope * log10f(ratio) + intercept;
    return powf(10.0f, log_ppm);
}

esp_err_t mq135_read(float temperature_c, float humidity_pct, mq135_sample_t *sample)
{
    if (!s_ready) {
        ESP_RETURN_ON_ERROR(mq135_init(10.0f), TAG, "init");
    }
    uint32_t accum = 0;
    const int samples = 32;
    for (int i = 0; i < samples; ++i) {
        accum += adc1_get_raw(EDGE_MQ135_ADC_CHANNEL);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    uint32_t avg = accum / samples;
    uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(avg, &s_chars);
    float rload = 10.0f; // kOhm
    float sensor_voltage = (float)voltage_mv / 1000.0f;
    float ratio = ((5.0f - sensor_voltage) / sensor_voltage) * rload / s_r0;
    float ppm = mq135_compute_ppm(ratio);
    // basic compensation
    ppm *= (1.0f + 0.02f * (temperature_c - 20.0f));
    ppm *= (1.0f - 0.01f * (humidity_pct - 50.0f));
    if (ppm < 0) {
        ppm = 0;
    }
    sample->ppm = ppm;
    sample->r0 = s_r0;
    return ESP_OK;
}
