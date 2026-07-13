/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#ifndef COMMON_H
#define COMMON_H

/* Includes */
/* STD APIs */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* ESP APIs */
#include "esp_log.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

/* FreeRTOS APIs */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/* NimBLE stack APIs */
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

/* Defines */
#define TAG "NimBLE_GATT_Server"
#define DEVICE_NAME "NimBLE_GATT"

typedef struct
{
    bool initialized;
    bool advertising;
    bool connected;
    bool pairing_mode;
    uint8_t own_addr_type;
    uint16_t conn_handle;

} ble_context_t;

#endif // COMMON_H
