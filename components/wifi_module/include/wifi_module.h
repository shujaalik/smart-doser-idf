#pragma once
#ifndef _WIFI_MODULE_H_
#define _WIFI_MODULE_H_

#include "main.h"
#include "mqtt_client.h"
#include "esp_err.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include <cJSON.h>
#include "esp_mac.h"

typedef struct
{
    char *pub;
    char *sub;
} topic_t;

void set_topic(void);
void init_wifi_module(void);
void wifi_start(void);
void publish(char *msg);
char *mac_address(void);
void mqtt_action_performer(char *msg);
void mqtt_app_start(void);
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event);
void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

#endif