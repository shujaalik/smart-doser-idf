#include "wifi_module.h"

// MQTT CERT
extern const uint8_t ca_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t ca_cert_pem_end[] asm("_binary_ca_cert_pem_end");

esp_mqtt_client_handle_t mqtt_client;
topic_t topics;

bool wifi_connected = false;
bool mqtt_connected = false;

void publish(char *msg)
{
    ESP_LOGI("MQTT", "Publishing message: %s", msg);
    int ret = esp_mqtt_client_publish(mqtt_client, topics.pub, msg, 0, 1, 0);
    if (ret == -1)
        ESP_LOGE("MQTT", "Failed to publish message");
}

char *mac_address(void)
{
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    char *mac_string = malloc(18);
    sprintf(mac_string, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return mac_string;
}

void set_topic(void)
{
    // sub to smart-doser-225/server_to_unit/MAC
    // pub to smart-doser-225/unit_to_server/MAC
    // broadcast_sub to smart-doser-225/server_to_unit/broadcast
    char *mac_str = mac_address();
    topics.pub = malloc(50);
    topics.sub = malloc(50);
    sprintf(topics.pub, "smart-doser-225/unit_to_server/%s", mac_str);
    sprintf(topics.sub, "smart-doser-225/server_to_unit/%s", mac_str);
    topics.broadcast_sub = malloc(50);
    sprintf(topics.broadcast_sub, "smart-doser-225/server_to_unit/broadcast");
    printf("PUB: %s\n", topics.pub);
    printf("SUB: %s\n", topics.sub);
    printf("BROADCAST_SUB: %s\n", topics.broadcast_sub);
}

void mqtt_action_performer(char *msg)
{
    // convert msg to json
    act(msg, publish);
}

esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    static const char *TAG = "MQTT_EVENT";
    esp_mqtt_client_handle_t client = event->client;
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        esp_mqtt_client_subscribe(client, topics.sub, 1);
        esp_mqtt_client_subscribe(client, topics.broadcast_sub, 1);
        ESP_LOGI(TAG, "Subscribed to topic: %s", topics.sub);
        ESP_LOGI(TAG, "Subscribed to topic: %s", topics.broadcast_sub);
        if (!mqtt_connected)
        {
            mqtt_connected = true;
        }
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        char *msg = malloc(event->data_len + 1);
        memcpy(msg, event->data, event->data_len);
        msg[event->data_len] = '\0';
        mqtt_action_performer(msg);
        free(msg);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
    return ESP_OK;
}

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    mqtt_event_handler_cb(event_data);
}
void mqtt_app_start(void)
{
    char *mac = mac_address();
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = "mqtts://mqtt.industrialpmr.com:8883",
            .verification.certificate = (const char *)ca_cert_pem_start,
        },
        .credentials = {
            .client_id = mac,
            .username = "ipmr-unit-mqtt",
            .authentication.password = "Kunjisoft1",
        }};
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, mqtt_client);
    esp_mqtt_client_start(mqtt_client);
}

void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    static const char *TAG = "WIFI_EVENT";
    if (event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "WIFI CONNECTING....\n");
    }
    else if (event_id == WIFI_EVENT_STA_CONNECTED)
    {
        wifi_connected = true;
        ESP_LOGI(TAG, "WiFi CONNECTED\n");
    }
    else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        wifi_connected = false;
        wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;
        // wifi SSID
        ESP_LOGI(TAG, "WiFi lost connection because of %d\n", event->reason);
        ESP_LOGI(TAG, "Retrying to Connect...\n");
        esp_wifi_connect();
    }
    else if (event_id == IP_EVENT_STA_GOT_IP)
    {
        ESP_LOGI(TAG, "Wifi got IP...\n\n");
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        const esp_netif_ip_info_t *ip_info = &event->ip_info;
        ESP_LOGI(TAG, "WIFIIP:" IPSTR, IP2STR(&ip_info->ip));
        ESP_LOGI(TAG, "WIFIMASK:" IPSTR, IP2STR(&ip_info->netmask));
        ESP_LOGI(TAG, "WIFIGW:" IPSTR, IP2STR(&ip_info->gw));
    }
    else
    {
        ESP_LOGI(TAG, "Unhandled WiFi event: %ld", event_id);
    }
}

void wifi_start(void)
{
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
    wifi_config_t wifi_configuration = {
        .sta.ssid = "",
        .sta.password = ""};
    strncpy((char *)wifi_configuration.sta.ssid, (char *)DEFAULT_SSID, 32);
    strncpy((char *)wifi_configuration.sta.password, (char *)DEFAULT_PASS, 64);
    wifi_configuration.sta.pmf_cfg.capable = true;
    wifi_configuration.sta.pmf_cfg.required = false;
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    esp_err_t err = esp_wifi_start();
    if (err != ESP_OK)
    {
        ESP_LOGE("WIFI_MODULE", "Failed to start WiFi: %s", esp_err_to_name(err));
        return;
    }
    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK)
    {
        ESP_LOGE("WIFI_MODULE", "Failed to set WiFi mode: %s", esp_err_to_name(err));
        return;
    }
    err = esp_wifi_connect();
    if (err != ESP_OK)
    {
        ESP_LOGE("WIFI_MODULE", "Failed to connect to WiFi: %s", esp_err_to_name(err));
        return;
    }
}

void start_wifi_module(void)
{
    wifi_start();
    vTaskDelay(pdMS_TO_TICKS(2000));
    mqtt_app_start();
    ESP_LOGI("WIFI_MODULE", "MQTT client started");
    ESP_LOGI("WIFI_MODULE", "WiFi module initialized successfully");
}

void init_wifi_module(void)
{
    const char *TAG = "WIFI_MODULE";
    nvs_flash_init();
    set_topic();
    esp_netif_init();
    esp_event_loop_create_default();

    esp_netif_t *netif = esp_netif_create_default_wifi_sta();
    esp_netif_set_hostname(netif, DEVICE_NAME);
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t err = esp_wifi_init(&wifi_initiation);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize WiFi: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "WiFi module initialized successfully");
}
