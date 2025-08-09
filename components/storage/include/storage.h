#pragma once
#ifndef _STORAGE_H_
#define _STORAGE_H_

#include "main.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_spiffs.h"
#include "cJSON.h"
#include "dirent.h"

void init_spiffs(void);
float get_storage_used_percent(void);
esp_err_t available_storage_fs(void);
void save_log(cJSON *log);
cJSON *read_log(void);
void delete_log(void);
void fetch_programs(void);
void save_programs(void);

#endif