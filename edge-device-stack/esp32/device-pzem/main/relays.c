#include "relays.h"

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "pins.h"

static const char *TAG = "pzem_relays";
static bool s_ready = false;

static gpio_num_t relay_gpio(pzem_relay_t relay)
{
    return (gpio_num_t)(EDGE_RELAY_GPIO_BASE + (int)relay);
}

esp_err_t pzem_relays_init(void)
{
    if (s_ready) {
        return ESP_OK;
    }
    for (int i = 0; i < 2; ++i) {
        gpio_num_t gpio = relay_gpio((pzem_relay_t)i);
        gpio_reset_pin(gpio);
        gpio_set_direction(gpio, GPIO_MODE_OUTPUT);
        gpio_set_level(gpio, 0);
    }
    s_ready = true;
    ESP_LOGI(TAG, "relays ready");
    return ESP_OK;
}

esp_err_t pzem_relays_set(pzem_relay_t relay, bool on)
{
    if (!s_ready) {
        ESP_RETURN_ON_ERROR(pzem_relays_init(), TAG, "init");
    }
    return gpio_set_level(relay_gpio(relay), on ? 1 : 0);
}

void pzem_relays_off(void)
{
    for (int i = 0; i < 2; ++i) {
        gpio_set_level(relay_gpio((pzem_relay_t)i), 0);
    }
}
