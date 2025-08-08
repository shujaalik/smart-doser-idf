#pragma once
#ifndef _MAIN_H_
#define _MAIN_H_

#include <stdio.h>
#include "driver/gpio.h"
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"

#define DEVICE_NAME "doser225"

#define DEBOUNCE_DURATION 100

#define BUTTON_01 GPIO_NUM_35
#define BUTTON_02 GPIO_NUM_36
#define BUTTON_03 GPIO_NUM_34

#define FULL_OPEN_SWITCH GPIO_NUM_19
#define FULL_CLOSED_SWITCH GPIO_NUM_18

#define DRIVER_IN1 GPIO_NUM_25
#define DRIVER_IN2 GPIO_NUM_26
#define DRIVER_IN3 GPIO_NUM_27
#define DRIVER_IN4 GPIO_NUM_33

#define DRIVER_STEPS_PER_REV 200
#define DRIVER_DEFAULT_SPEED 100
#define DRIVER_STEPS_PER_ML 90

#define LCD_ADDR 0x27
#define LCD_NUM_ROWS 4
#define LCD_NUM_COLS 16

#define DEFAULT_SSID "industrialpmr"
#define DEFAULT_PASS "kunjisoftadmin"

typedef struct
{
    char *action;
    char *data;
    char *secondary_data;
} cmd_t;

typedef enum
{
    MAIN_MENU,
    MANUAL_CONTROL,
    SCHEDULED_DOSE,
    PURGE_UNPURGE
} page_t;

typedef enum
{
    MANUAL_MODE,
    SCHEDULED_MODE,
    PURGE_MODE
} main_select_t;

typedef struct
{
    float flow_rate; // in ml/h
    float volume;    // in ml
} manual_control_t;

typedef enum
{
    CURSOR_FLOW_RATE,
    CURSOR_VOLUME,
    CURSOR_START_STOP,
    CURSOR_HOME
} manual_control_cursor_t;

typedef struct
{
    float vtbi;      // Volume to be infused in ml
    float flow_rate; // Flow rate in ml/h
    int duration;    // Duration in seconds
} scheduled_control_t;

typedef struct
{
    gpio_num_t in1, in2, in3, in4;
    gpio_num_t full_open_switch, full_closed_switch;
    int number_of_steps;
    int step_number;
    int steps_left;
    uint64_t last_step_time;
    uint32_t step_delay;
    uint32_t max_speed_delay;
} StepperMotor;

void initialize_all(void);
void act(char *cmd_json, void (*callback)(const char *));

#include "time_manager.h"
#include "lcd1604.h"
#include "io_module.h"
#include "bt_module.h"
#include "wifi_module.h"
#include "stepper.h"
#include "doser.h"
#include <cJSON.h>

#endif