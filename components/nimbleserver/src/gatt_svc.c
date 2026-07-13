/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* Includes */
#include "gap.h"
#include "gatt_svc.h"
#include "common.h"
#include "ble_gatt_svc_uuid16.h"
#include "ble.h"
#include "heart_rate.h"
#include "led.h"

/* Private function declarations */
static int command_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg);

/* Private variables */
static api_post_command_from_ble_t api_post_command = NULL;
// Automation IO service
static const ble_uuid16_t auto_io_svc_uuid = BLE_UUID16_INIT(BLE_GATT_SVC_UUID16_VALUE);

// command
static uint16_t command_chr_val_handle;
static const ble_uuid16_t command_chr_uuid = BLE_UUID16_INIT(0xFF02);

// response
static uint8_t response_chr_val[CONFIG_NIMBLE_BUFFER_SIZE] = {0};
static uint16_t response_chr_val_len = sizeof(response_chr_val);
static uint16_t response_chr_val_handle;
static const ble_uuid16_t response_chr_uuid = BLE_UUID16_INIT(0xFF01);
static int response_chr_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);

// device information
static uint8_t devstatus_chr_val[CONFIG_NIMBLE_BUFFER_SIZE] = {0};
static uint16_t devstatus_chr_val_len = sizeof(devstatus_chr_val);
static uint16_t devstatus_chr_val_handle;
static const ble_uuid16_t devstatus_chr_uuid = BLE_UUID16_INIT(0xFF03);
static int devstatus_chr_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);

static int gatt_svr_chr_user_desc_cb(uint16_t conn_handle,
    uint16_t attr_handle,
    struct ble_gatt_access_ctxt *ctxt,
    void *arg);

/* GATT services table */
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    // Automation IO service
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &auto_io_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]){
            {.uuid = &command_chr_uuid.u,   // Command characteristic
                .access_cb = command_chr_access,
                .flags = BLE_GATT_CHR_F_WRITE,
                .val_handle = &command_chr_val_handle,
                .descriptors = (struct ble_gatt_dsc_def[]) {
                    {
                        .uuid = BLE_UUID16_DECLARE(0x2901),
                        .att_flags = BLE_ATT_F_READ,
                        .access_cb = gatt_svr_chr_user_desc_cb,
                        .arg = "Command input"
                    },
                    {0} // No more descriptors
                }
            },
            {
                .uuid = &response_chr_uuid.u,   // response characteristic
                .access_cb = response_chr_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_INDICATE,
                .val_handle = &response_chr_val_handle,
                .descriptors = (struct ble_gatt_dsc_def[]) {
                    {
                        .uuid = BLE_UUID16_DECLARE(0x2901),
                        .att_flags = BLE_ATT_F_READ,
                        .access_cb = gatt_svr_chr_user_desc_cb,
                        .arg = "Device response"
                    },
                    {0} // No more descriptors
                }
            },
            {
                .uuid = &devstatus_chr_uuid.u,  // devstatus characteristic
                .access_cb = devstatus_chr_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_INDICATE,
                .val_handle = &devstatus_chr_val_handle,
                .descriptors = (struct ble_gatt_dsc_def[]) {
                    {
                        .uuid = BLE_UUID16_DECLARE(0x2901),
                        .att_flags = BLE_ATT_F_READ,
                        .access_cb = gatt_svr_chr_user_desc_cb,
                        .arg = "Device status"
                    },
                    {0} // No more descriptors
                }
            },
            {0} // No more characteristics
        },
    },

    {
        0, // No more services
    },
};

static int gatt_svr_chr_user_desc_cb(uint16_t conn_handle,
                          uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt,
                          void *arg)
{
    const char *desc = (const char *)arg;
    return os_mbuf_append(ctxt->om, desc, strlen(desc)) == 0
               ? 0
               : BLE_ATT_ERR_INSUFFICIENT_RES;
}

void nimble_set_callbacks(api_post_command_from_ble_t command)
{
    // Set callbacks for API command and response
    api_post_command = command;
}

/* Private functions */
static int command_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    // Local variables
    int rc = ESP_OK;

    // Handle access events
    // Note: Command characteristic is write only
    switch (ctxt->op) {

    // Write characteristic event
    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        // Verify connection handle
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI(TAG, "characteristic write; conn_handle=%d attr_handle=%d",
                     conn_handle, attr_handle);
        } else {
            ESP_LOGI(TAG,
                     "characteristic write by nimble stack; attr_handle=%d",
                     attr_handle);
        }

        // Verify attribute handle
        if (attr_handle == command_chr_val_handle) {
            ESP_LOGI(TAG, "ctxt->om->om_len = %d",ctxt->om->om_len);
            // Verify access buffer length
            if (ctxt->om->om_len > 0) {
                if (api_post_command != NULL) {
                    rc = api_post_command(ctxt->om);
                    if (rc != ESP_OK) {
                        ESP_LOGE(TAG,"api_post_command failed, error code: %d, GATT return code: %d", rc, BLE_ATT_ERR_UNLIKELY);
                        return BLE_ATT_ERR_UNLIKELY;
                    }
                }
            } else {
                goto error;
            }
            return rc;
        }
        goto error;

    // Unknown event
    default:
        goto error;
    }

error:
    ESP_LOGE(TAG,
             "unexpected access operation to command characteristic, opcode: %d",
             ctxt->op);
    return BLE_ATT_ERR_UNLIKELY;
}

static int response_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    int rc = ESP_OK;

    // Handle access events
    // Note: Status characteristic is read only
    switch (ctxt->op) {

    // Read characteristic event
    case BLE_GATT_ACCESS_OP_READ_CHR:
        // Verify connection handle
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI(TAG, "characteristic read; conn_handle=%d attr_handle=%d", conn_handle, attr_handle);
        }
        else {
            ESP_LOGI(TAG, "characteristic read by nimble stack; attr_handle=%d", attr_handle);
        }

        // Verify attribute handle
        if (attr_handle == response_chr_val_handle) {
            // Update access buffer value
            rc = os_mbuf_append(ctxt->om, &response_chr_val, ((response_chr_val_len <= sizeof(response_chr_val)) ? response_chr_val_len : sizeof(response_chr_val)));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        goto error;

    // Unknown event
    default:
        goto error;
    }
    error:
    ESP_LOGE(TAG, "unexpected access operation to response characteristic, opcode: %d", ctxt->op);
    return BLE_ATT_ERR_UNLIKELY;
}

static int devstatus_chr_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    int rc = ESP_OK;

    // Handle access events
    // Note: Status characteristic is read only
    switch (ctxt->op) {

    // Read characteristic event
    case BLE_GATT_ACCESS_OP_READ_CHR:
        // Verify connection handle
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI(TAG, "characteristic read; conn_handle=%d attr_handle=%d", conn_handle, attr_handle);
        }
        else {
            ESP_LOGI(TAG, "characteristic read by nimble stack; attr_handle=%d", attr_handle);
        }

        // Verify attribute handle
        if (attr_handle == devstatus_chr_val_handle) {
            // Update access buffer value
            rc = os_mbuf_append(ctxt->om, &devstatus_chr_val, ((devstatus_chr_val_len <= sizeof(devstatus_chr_val)) ? devstatus_chr_val_len : sizeof(devstatus_chr_val)));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        goto error;

    // Unknown event
    default:
        goto error;
    }
    error:
    ESP_LOGE(TAG,
             "unexpected access operation to devstatus characteristic, opcode: %d", ctxt->op);
    return BLE_ATT_ERR_UNLIKELY;
}

/* Public functions */
/*
 *  Handle GATT attribute register events
 *      - Service register event
 *      - Characteristic register event
 *      - Descriptor register event
 */
void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg) {
    /* Local variables */
    char buf[BLE_UUID_STR_LEN];

    /* Handle GATT attributes register events */
    switch (ctxt->op) {

    /* Service register event */
    case BLE_GATT_REGISTER_OP_SVC:
        ESP_LOGD(TAG, "registered service %s with handle=%d",
                 ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                 ctxt->svc.handle);
        break;

    /* Characteristic register event */
    case BLE_GATT_REGISTER_OP_CHR:
        ESP_LOGD(TAG,
                 "registering characteristic %s with "
                 "def_handle=%d val_handle=%d",
                 ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                 ctxt->chr.def_handle, ctxt->chr.val_handle);
        break;

    /* Descriptor register event */
    case BLE_GATT_REGISTER_OP_DSC:
        ESP_LOGD(TAG, "registering descriptor %s with handle=%d",
                 ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                 ctxt->dsc.handle);
        break;

    /* Unknown event */
    default:
        assert(0);
        break;
    }
}

/*
 *  GATT server subscribe event callback
 *      1. Update heart rate subscription status
 */

void gatt_svr_subscribe_cb(struct ble_gap_event *event) {
    /* Check connection handle */
    if (event->subscribe.conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGI(TAG, "subscribe event; conn_handle=%d attr_handle=%d",
                 event->subscribe.conn_handle, event->subscribe.attr_handle);
    } else {
        ESP_LOGI(TAG, "subscribe by nimble stack; attr_handle=%d",
                 event->subscribe.attr_handle);
    }
}

/*
 *  GATT server initialization
 *      1. Initialize GATT service
 *      2. Update NimBLE host GATT services counter
 *      3. Add GATT services to server
 */
int gatt_svc_init(void) {
    /* Local variables */
    int rc = 0;

    /* 1. GATT service initialization */
    ble_svc_gatt_init();

    /* 2. Update GATT services counter */
    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    /* 3. Add GATT services */
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

static esp_err_t ble_indicate(uint8_t* buffer, uint16_t len, uint8_t* chr_val, uint16_t chr_val_size, uint16_t* chr_val_len, uint16_t chr_val_handle)
{
    int rc = 0;
    struct os_mbuf *om = NULL;

    uint16_t conn_handle = get_ble_conn_handle();

    if (conn_handle == BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGE(TAG, "invalid connection handle or not connected");
        return ESP_FAIL;
    }

    // Check buffer length
    if (len > chr_val_size) {
        ESP_LOGE(TAG, "buffer length is too long!");
        return ESP_FAIL;
    }
    *chr_val_len = len;
    memcpy(chr_val, buffer, len);

    // Create os_mbuf for indication
    om = ble_hs_mbuf_from_flat(buffer, len);
    if (om == NULL) {
        ESP_LOGE(TAG, "failed to create os_mbuf for indication!");
        return ESP_FAIL;
    }

    // Send indication
    rc = ble_gatts_indicate_custom(conn_handle, chr_val_handle, om);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to send indication, error code: %d", rc);
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t ble_indicate_response(uint8_t* buffer, uint16_t len)
{
    return ble_indicate(buffer, len, response_chr_val, sizeof(response_chr_val), &response_chr_val_len, response_chr_val_handle);
}

esp_err_t ble_indicate_devstatus(uint8_t* buffer, uint16_t len)
{
    return ble_indicate(buffer, len, devstatus_chr_val, sizeof(devstatus_chr_val), &devstatus_chr_val_len, devstatus_chr_val_handle);
}
