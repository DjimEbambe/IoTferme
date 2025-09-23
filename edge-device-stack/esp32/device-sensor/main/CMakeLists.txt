idf_component_register(
    SRCS
        "app_main.c"
        "sht41.c"
        "light_adc.c"
        "mq135_adc.c"
        "relays.c"
        "config_cli.c"
    INCLUDE_DIRS "."
    REQUIRES
        driver
        esp_timer
        esp_wifi
        esp_event
        common
        cJSON
)
