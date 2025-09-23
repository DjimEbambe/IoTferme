idf_component_register(
    SRCS
        "gateway_main.c"
        "usb_cdc.c"
        "router.c"
        "ack_table.c"
        "provisioning.c"
    INCLUDE_DIRS "."
    REQUIRES
        esp_timer
        esp_wifi
        esp_event
        esp_tinyusb
        common
        json
)
