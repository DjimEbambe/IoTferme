#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t wifi_init_sta(const char *ssid, const char *pass);
void wifi_wait_for_ip(void);

#ifdef __cplusplus
}
#endif
