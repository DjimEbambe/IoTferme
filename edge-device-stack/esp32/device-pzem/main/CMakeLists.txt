idf_component_register(
    SRCS
        "app_main.c"
        "pzem.c"
        "relays.c"
    INCLUDE_DIRS "."
    REQUIRES
        driver
        esp_timer
        esp_wifi
        esp_event
        common
        cJSON
)
