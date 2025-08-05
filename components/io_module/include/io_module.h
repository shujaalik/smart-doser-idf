#pragma once
#ifndef _IO_MODULE_H_
#define _IO_MODULE_H_

#include "main.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_timer.h"
#include "driver/gpio.h"

#define ESP_INTR_FLAG_DEFAULT 0

void io_module_init(void); // Initialize the IO module

#endif