#include "relays.h"

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "pins.h"

static const char *TAG = "relays";
static bool s_ready = false;

static gpio_num_t relay_gpio(relay_channel_t channel)
{
    return (gpio_num_t)(EDGE_RELAY_GPIO_BASE + (int)channel);
}

esp_err_t relays_init(void)
{
    if (s_ready) {
        return ESP_OK;
    }
    for (int i = 0; i < EDGE_RELAY_COUNT; ++i) {
        gpio_num_t gpio = (gpio_num_t)(EDGE_RELAY_GPIO_BASE + i);
        gpio_reset_pin(gpio);
        gpio_set_direction(gpio, GPIO_MODE_OUTPUT);
        gpio_set_level(gpio, 0);
    }
    s_ready = true;
    ESP_LOGI(TAG, "relays ready");
    return ESP_OK;
}

esp_err_t relays_set(relay_channel_t channel, bool on)
{
    if (!s_ready) {
        ESP_RETURN_ON_ERROR(relays_init(), TAG, "init");
    }
    gpio_num_t gpio = relay_gpio(channel);
    return gpio_set_level(gpio, on ? 1 : 0);
}

void relays_all_off(void)
{
    for (int i = 0; i < EDGE_RELAY_COUNT; ++i) {
        gpio_set_level(relay_gpio((relay_channel_t)i), 0);
    }
}
