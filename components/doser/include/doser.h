#pragma once
#ifndef _DOSER_H_
#define _DOSER_H_

#include "main.h"
#include "stepper.h"

typedef struct
{
    StepperMotor motor;
    float calibration_factor;
} Doser;

void doser_init(Doser *doser, int steps_per_rev, float max_rpm, float calibration_factor);
void doser_dispense(Doser *doser, float quantity_ml, float speed_rpm);
void doser_stop(Doser *doser);
void doser_run_program(Doser *doser, float vtbi, float flow_rate);

#endif