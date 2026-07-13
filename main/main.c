/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* Includes */
#include "common.h"
#include "gap.h"
#include "gatt_svc.h"
#include "ble.h"
#include "heart_rate.h"
#include "led.h"

static void heart_rate_task(void *param) {
    /* Task entry log */
    ESP_LOGI(TAG, "heart rate task has been started!");

    /* Loop forever */
    while (1) {
        /* Update heart rate value every 1 second */
        update_heart_rate();
        ESP_LOGI(TAG, "heart rate updated to %d", get_heart_rate());

        /* Send heart rate indication if enabled */
        send_heart_rate_indication();

        /* Sleep */
        vTaskDelay(HEART_RATE_TASK_PERIOD);
    }

    /* Clean up at exit */
    vTaskDelete(NULL);
}

void app_main(void) {
    /* Local variables */
    int rc = 0;
    esp_err_t ret;

    /* LED initialization */
    led_init();

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
    xTaskCreate(heart_rate_task, "Heart Rate", 4*1024, NULL, 5, NULL);

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
