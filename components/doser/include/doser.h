#pragma once
#ifndef _DOSER_H_
#define _DOSER_H_

#include "main.h"
#include "stepper.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef struct
{
    StepperMotor motor;
    float calibration_factor;
} Doser;
typedef struct
{
    Doser *doser;
    int steps_needed;
    float speed_rpm;
} DoserDispenseTaskArgs;

void doser_init(Doser *doser, int steps_per_rev, float max_rpm, float calibration_factor);
void doser_dispense(Doser *doser, float quantity_ml, float speed_rpm);
void doser_stop(Doser *doser);
void doser_run_program(Doser *doser, float vtbi, float flow_rate, int print);
void doser_full_open(Doser *doser);
void doser_full_close(Doser *doser);

#endif