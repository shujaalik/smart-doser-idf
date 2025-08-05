#pragma once
#ifndef _BT_MODULE_H_
#define _BT_MODULE_H_

#include "main.h"
#include "esp_nimble_hci.h"
#include "esp_timer.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include <cJSON.h>

void send_ble(uint16_t conn_handle, char *msg);
void send_ble_notification(uint16_t conn_handle, char *msg);
int device_read(uint16_t con_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
int device_write(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
int ble_gap_event(struct ble_gap_event *event, void *arg);
void ble_app_advertise(void);
void ble_app_on_sync(void);
void init_ble_module(void);
void host_task(void *param);
void start_bt_module(void);

#endif