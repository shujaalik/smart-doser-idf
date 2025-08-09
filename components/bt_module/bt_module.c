#include "bt_module.h"

struct msgWithLen_t
{
    char *msg;
    int len;
} msg_from_server;
bool bt_connected = false;

TaskHandle_t ble_handle_task;
uint16_t control_notif_handle;
static uint16_t ble_conn_handle = BLE_HS_CONN_HANDLE_NONE;
uint8_t ble_addr_type;

void send_ble(uint16_t conn_handle, char *msg)
{
    int len = strlen(msg);
    struct os_mbuf *response_mbuf = ble_hs_mbuf_from_flat(msg, len);
    int rc = ble_gattc_notify_custom(conn_handle, control_notif_handle, response_mbuf);
    if (rc != 0)
        printf("Failed to send notification: %d\n", rc);
    else
        printf("Sent notification: %s\n", msg);
}
void send_ble_notification(uint16_t conn_handle, char *msg)
{
    int len = strlen(msg);
    // Send a notification to the client
    // if length of msg is grater than 80, then split the msg into lengths of 149 and keep sending with & at the end and nothing at the end of the last msg
    if (len > 150)
    {
        int i = 0;
        while (i < len)
        {
            char *msg_part = malloc(151);
            // in msg_part, copy 79 characters from msg and add & at the end(only if it is not the last msg)
            if (i + 149 < len)
            {
                strncpy(msg_part, msg + i, 149);
                msg_part[149] = '&';
                msg_part[150] = '\0';
            }
            else
            {
                strncpy(msg_part, msg + i, len - i);
                msg_part[len - i] = '\0';
            }
            send_ble(conn_handle, msg_part);
            free(msg_part);
            i += 149;
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
    else
        send_ble(conn_handle, msg);
}

int device_read(uint16_t con_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    return 0;
};
static void send_ble_notification_handler(char *msg)
{
    if (bt_connected)
    {
        send_ble_notification(ble_conn_handle, msg);
    }
    else
    {
        printf("BLE not connected, cannot send notification: %s\n", msg);
    }
}
int device_write(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char *msg = (char *)ctxt->om->om_data;
    act(msg, send_ble_notification_handler);
    return 0;
}

const struct ble_gatt_svc_def gatt_svcs[] = {
    {.type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = BLE_UUID16_DECLARE(0x0001), // Define UUID for device type
     .characteristics = (struct ble_gatt_chr_def[]){
         {.uuid = BLE_UUID16_DECLARE(0x0002), // Define UUID for reading
          .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
          .val_handle = &control_notif_handle,
          .access_cb = device_read},
         {.uuid = BLE_UUID16_DECLARE(0x0003), // Define UUID for writing
          .flags = BLE_GATT_CHR_F_WRITE,
          .access_cb = device_write},
         {0}}},
    {0}};
int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
    // Advertise if connected
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI("GAP", "BLE GAP EVENT CONNECT %s", event->connect.status == 0 ? "OK!" : "FAILED!");
        if (event->connect.status != 0)
        {
            ble_app_advertise();
        }
        ble_conn_handle = event->connect.conn_handle;
        bt_connected = true;
        break;
    // Advertise again after completion of the event
    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI("GAP", "BLE GAP EVENT DISCONNECTED");
        ble_conn_handle = BLE_HS_CONN_HANDLE_NONE;
        bt_connected = false;
        ble_app_advertise();
        break;
    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI("GAP", "BLE GAP EVENT");
        ble_app_advertise();
        break;
    case BLE_GAP_EVENT_SUBSCRIBE:
        // Subscribe to the event
        ESP_LOGI("GAP", "BLE GAP EVENT SUBSCRIBE");
        break;
    default:
        break;
    }
    return 0;
}
void ble_app_advertise(void)
{
    // GAP - device name definition
    struct ble_hs_adv_fields fields;
    const char *device_name;
    memset(&fields, 0, sizeof(fields));
    device_name = ble_svc_gap_device_name(); // Read the BLE device name
    fields.name = (uint8_t *)device_name;
    fields.name_len = strlen(device_name);
    fields.name_is_complete = 1;
    ble_gap_adv_set_fields(&fields);
    msg_from_server.len = 0;
    msg_from_server.msg = "\0";

    // GAP - device connectivity definition
    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND; // connectable or non-connectable
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN; // discoverable or non-discoverable
    ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL);
}
void ble_app_on_sync(void)
{
    ble_hs_id_infer_auto(0, &ble_addr_type); // Determines the best address type automatically
    ble_app_advertise();                     // Define the BLE connection
}
void host_task(void *param)
{
    nimble_port_run();
}

void init_ble_module(void)
{
    nimble_port_init();
    ble_svc_gap_device_name_set(DEVICE_NAME);
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);
    ble_hs_cfg.sync_cb = ble_app_on_sync;
    nimble_port_freertos_init(host_task);
}

void start_bt_module(void)
{
    init_ble_module();
}
