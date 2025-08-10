#pragma once
#ifndef _DOSER_H_
#define _DOSER_H_

#include "main.h"
#include "stepper.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd1604.h"

typedef struct
{
    StepperMotor motor;
    float calibration_factor;
} Doser;
typedef struct
{
    Doser *doser;
    int vtbi_ml;
    float flow_rate_ml_h;
    int print;
} DoserDispenseTaskArgs;

void doser_init(Doser *doser, int steps_per_rev, float calibration_factor);
void doser_dispense(Doser *doser, float quantity_ml, float flow_rate_ml_h, int print);
void doser_stop(Doser *doser);
void doser_run_program(Doser *doser, float vtbi_ml, float flow_rate_ml_h, int print);
void doser_full_open(Doser *doser);
void doser_full_close(Doser *doser);

#endif