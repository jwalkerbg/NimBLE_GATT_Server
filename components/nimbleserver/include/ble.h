// ble.h

#pragma once

#include <esp_err.h>

#if defined(__cplusplus)
extern "C" {
#endif

#include <os/os_mbuf.h>
#include <esp_err.h>

typedef esp_err_t (*api_post_command_from_ble_t)(struct os_mbuf *param);

esp_err_t ble_init(void);

void ble_set_callbacks(api_post_command_from_ble_t command);
esp_err_t ble_indicate_response(uint8_t* buffer, uint16_t len);
esp_err_t ble_indicate_devstatus(uint8_t* buffer, uint16_t len);

#if defined(__cplusplus)
}
#endif

// end of ble.h
