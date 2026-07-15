/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* Includes */
#include "common.h"
#include "ble.h"

static uint8_t buf[CONFIG_NIMBLE_BUFFER_SIZE] = {0};

#define MS_RESPONSE BIT0
#define MS_EXIT     BIT1
static EventGroupHandle_t ms_event_group = NULL;

esp_err_t command_cb (struct os_mbuf *param)
{
    int len = OS_MBUF_PKTLEN(param);
    if (len > sizeof(buf)) {
        len = sizeof(buf);
    }
    os_mbuf_copydata(param, 0, len, buf);
    ESP_LOGI(TAG, "WRITE received, length=%d", len);
    ESP_LOG_BUFFER_HEX(TAG, buf, len);

    xEventGroupSetBits(ms_event_group, MS_RESPONSE);

    return ESP_OK;
}

static TaskHandle_t ms_response_task_handle = NULL;
static void ms_response_task(void* pvParameters)
{
    ESP_LOGI(TAG,"ms_response_task entered");

    while (true) {
        EventBits_t bits = xEventGroupWaitBits(ms_event_group, MS_RESPONSE | MS_EXIT, pdTRUE, pdFALSE, portMAX_DELAY);
        if (bits & MS_RESPONSE) {
            static uint8_t buff[10] = {0};
            vTaskDelay(pdMS_TO_TICKS(1 * 1000));
            buff[0]++;
            ble_indicate_response(buff,1);
            ESP_LOGI(TAG, "Response sent");
        }
        if (bits & MS_EXIT) {
            break;
        }
    }

    vEventGroupDelete(ms_event_group);
    ms_event_group = NULL;
    ESP_LOGI(TAG,"ms_response_task exited");
    vTaskDelete(NULL);
}
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
    ble_set_callbacks(command_cb);
    ms_event_group = xEventGroupCreate();
    BaseType_t tret = xTaskCreate(ms_response_task,"co_sensor_periodic_task",5120,NULL,1,&ms_response_task_handle);
    if (tret == pdPASS) {
        ESP_LOGI(TAG,"Created ms_response_task");
    }
    else {
        ESP_LOGE(TAG,"Creating ms_response_task not successful");
    }

again:
    ESP_LOGI(TAG, "Waiting for %d seconds",CONFIG_SILENT_TIME);
    vTaskDelay(pdMS_TO_TICKS(CONFIG_SILENT_TIME * 1000));
    ESP_LOGI(TAG, "Entering pairing mode for %d seconds",CONFIG_PAIRING_TIME);
    ble_enter_pairing_mode();
    vTaskDelay(pdMS_TO_TICKS(CONFIG_PAIRING_TIME * 1000));
    ble_exit_pairing_mode();
    goto again;

    return;
}
