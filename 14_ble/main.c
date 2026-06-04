#include <string.h>
#include "esp_log.h"
#include "driver/gpio.h"

#include "nimble/nimble_port.h"             // init and start NimBLE on esp32c3 
#include "nimble/nimble_port_freertos.h"    // launch NimBLE host in task
#include "host/ble_hs.h"                    // BLE logic
#include "services/gap/ble_svc_gap.h"       // GAP service (Generic Access Profile)
#include "services/gatt/ble_svc_gatt.h"     // GATT (Generic Attribute Service)

static const char *TAG = "BLE_LED";

#define LED_GPIO 3
static uint8_t g_led_state = 0;

static uint8_t own_addr_type;

// ========  UUID (Universally Unique Identifier) ======================
#define SVC_UUID16 0xFFF0 // service
#define CHR_UUID16 0xFFF1 // characteristic

////////  GATT access callback  //////////////////////
static int led_chr_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                        struct ble_gatt_access_ctxt *ctxt, void *arg) {
    (void)conn_handle;
    (void)attr_handle;
    (void)arg;

    if(ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR){
        return BLE_ATT_ERR_UNLIKELY;
    }

    if(ctxt->om->om_len < 1){
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    uint8_t v = (ctxt->om->om_data[0] != 0) ? 1 : 0; // 01 -> led on; 00 -> led off
    g_led_state = v;

    gpio_set_level(LED_GPIO, g_led_state);
    ESP_LOGI(TAG, "Led state = %d", g_led_state);
    return 0;
}
// ======================================================

////////  Services table  //////////////////////
static struct ble_gatt_chr_def gatt_chars[] = {
    {
        .uuid = BLE_UUID16_DECLARE(CHR_UUID16),
        .access_cb = led_chr_access_cb,
        .flags = BLE_GATT_CHR_F_WRITE,
    },
    {0}
};

static struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(SVC_UUID16),
        .characteristics = gatt_chars,
    },
    {0}
};
// ====================================================

////// Advertising  ////////////////////////////////
static void start_advertising(void);

static int gap_event_cb(struct ble_gap_event *event, void *arg) {
    (void)arg;

    switch(event->type){
        case BLE_GAP_EVENT_CONNECT:
            if(event->connect.status == 0){
                ESP_LOGI(TAG, "Connected");
            } else {
                ESP_LOGW(TAG, "Connect failed status=%d", event->connect.status);
                start_advertising();
            }
            return 0;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "Disconnected; reason=%d", event->disconnect.reason);
            start_advertising();
            return 0;

        default:
            return 0;
    }
}

static void start_advertising(void){
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;

    memset(&fields, 0, sizeof(fields));

    const char *name = ble_svc_gap_device_name();
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    int rc = ble_gap_adv_set_fields(&fields); // Nimble try to set Payload form &fields
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to set advertising data, error code: %d", rc);
        return;
    }

    memset(&adv_params, 0, sizeof(adv_params));

    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    /* Start advertising */
    rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER, &adv_params,
                        gap_event_cb, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to start advertising, error code: %d", rc);
    } else ESP_LOGI(TAG, "advertising started!");
}
// ==============================================

////// ble sync  /////////////
static void ble_on_sync(void){
    int rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0){
        ESP_LOGE(TAG, "ble_hs_id_infer_auto rc=%d", rc);
        return;
    }
    start_advertising();
}
// =============================

/////// NimBLE host task  ////////////////////
static void host_task(void *param){
    (void)param;

    nimble_port_run();
    nimble_port_freertos_deinit();
}
// =======================================

void app_main() {
    // Gpio init
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io);
    gpio_set_level(LED_GPIO, 0);

    // NimBLE init
    nimble_port_init();
    
    // Standard GAP/GATT services
    ble_svc_gap_init();
    ble_svc_gatt_init();

    ble_svc_gap_device_name_set("ESP32C3-LED");

    // Our custom service
    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);

    // Start advertising after sync
    ble_hs_cfg.sync_cb = ble_on_sync;

    nimble_port_freertos_init(host_task);

    ESP_LOGI(TAG, "BLE ready. Connect and write 01/00 to char 0xFFF1");
}
