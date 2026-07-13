/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* Includes */
#include "common.h"
#include "gap.h"
#include "ble.h"

void app_main(void)
{
    esp_err_t ret;

    /*
     * NVS flash initialization
     * Dependency of BLE stack to store configurations
     */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to initialize nvs flash, error code: %d ", ret);
        return;
    }

    /* NimBLE stack initialization */
    ble_init();

again:
    ESP_LOGI(TAG, "Waiting for 30 seconds");
    vTaskDelay(pdMS_TO_TICKS(60000));
    ESP_LOGI(TAG, "Entering pairing mode");
    ble_enter_pairing_mode();
    vTaskDelay(pdMS_TO_TICKS(120000));
    ble_exit_pairing_mode();
    goto again;

    return;
}
