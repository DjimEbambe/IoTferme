idf_component_register(
    SRCS
        "src/frames.c"
        "src/time_sync.c"
        "src/nvs_store.c"
        "src/wifi_init.c"
        "src/espnow_init.c"
        "src/crc.c"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash esp_wifi esp_event esp_timer driver json mbedtls
)

idf_build_set_property(COMPILE_DEFINITIONS "CONFIG_EDGE_IDF" APPEND)
