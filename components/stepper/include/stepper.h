#pragma once
#ifndef _STEPPER_H_
#define _STEPPER_H_

#include "main.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include <stdint.h>
#include <stdbool.h>

void stepper_release(StepperMotor *motor);
void stepper_set_speed(StepperMotor *motor, float rpm);
void stepper_step_motor(StepperMotor *motor, int step);
void stepper_init(StepperMotor *motor, gpio_num_t in1, gpio_num_t in2, gpio_num_t in3, gpio_num_t in4, int steps, float max_speed_rpm);
void stepper_stop(StepperMotor *motor);
void stepper_step(StepperMotor *motor, int steps_to_move, float speed_rpm);

#endif