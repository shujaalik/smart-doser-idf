#include "doser.h"

void doser_init(Doser *doser, int steps_per_rev, float max_rpm, float calibration_factor)
{
    // Initialize the stepper motor within the doser structure
    stepper_init(&doser->motor, doser->motor.in1, doser->motor.in2, doser->motor.in3, doser->motor.in4, doser->motor.full_open_switch, doser->motor.full_closed_switch, steps_per_rev, max_rpm);

    // Set additional parameters for the doser
    doser->calibration_factor = calibration_factor; // steps per ml
}

void doser_dispense(Doser *doser, float quantity_ml, float speed_rpm)
{
    // Calculate required steps using calibration factor (steps/ml)
    int steps_needed = (int)(quantity_ml * doser->calibration_factor);
    stepper_step(&doser->motor, steps_needed, speed_rpm);
    ESP_LOGI("Doser", "Dispensed %.2f ml, which is %d steps at %.2f RPM", quantity_ml, steps_needed, speed_rpm);
}

void doser_run_program(Doser *doser, float vtbi, float flow_rate)
{
    int steps_needed = (int)(vtbi * doser->calibration_factor);
    float speed_rpm = flow_rate / doser->calibration_factor * 60;
    stepper_step(&doser->motor, steps_needed, speed_rpm);
    ESP_LOGI("Doser", "Running program: VTBI=%.2f ml, Flow Rate=%.2f ml/h, Steps=%d, Speed=%.2f RPM", vtbi, flow_rate, steps_needed, speed_rpm);
}

void doser_stop(Doser *doser)
{
    stepper_stop(&doser->motor);
}

void full_open(Doser *doser)
{
    doser_dispense(doser, 1000.0f, 60.0f); // Dispense 1000 ml at 60 RPM
}

void full_close(Doser *doser)
{
    doser_dispense(doser, -1000.0f, 60.0f); // Stop dispensing
}