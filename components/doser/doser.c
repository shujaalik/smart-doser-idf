#include "doser.h"

void doser_init(Doser *doser, int steps_per_rev, float calibration_factor)
{
    // Initialize the stepper motor within the doser structure
    stepper_init(&doser->motor,
                 doser->motor.in1,
                 doser->motor.in2,
                 doser->motor.in3,
                 doser->motor.in4,
                 doser->motor.full_open_switch,
                 doser->motor.full_closed_switch,
                 steps_per_rev);

    // Set additional parameters for the doser
    doser->calibration_factor = calibration_factor; // steps per ml
    doser_full_open(doser);                         // Move to full open position initially
}

void doser_dispense(Doser *doser, float quantity_ml, float flow_rate_ml_h, int print)
{
    // Use ml/h directly
    stepper_step_ml(&doser->motor, quantity_ml, flow_rate_ml_h, print);
    ESP_LOGI("Doser", "Dispensed %.2f ml at %.2f ml/h", quantity_ml, flow_rate_ml_h);
    main_screen();
}

static void doser_dispense_task(void *param)
{
    DoserDispenseTaskArgs *args = (DoserDispenseTaskArgs *)param;
    stepper_step_ml(&args->doser->motor, args->vtbi_ml, args->flow_rate_ml_h, args->print);
    vPortFree(args);
    vTaskDelete(NULL);
    main_screen();
}

void doser_run_program(Doser *doser, float vtbi_ml, float flow_rate_ml_h, int print)
{
    DoserDispenseTaskArgs *args = pvPortMalloc(sizeof(DoserDispenseTaskArgs));
    if (args)
    {
        args->doser = doser;
        args->vtbi_ml = vtbi_ml;
        args->flow_rate_ml_h = flow_rate_ml_h;
        args->print = print;
        xTaskCreate(doser_dispense_task, "doser_dispense_task", 2048 * 2, args, 5, NULL);
    }
    else
    {
        ESP_LOGE("Doser", "Failed to allocate memory for dispense task");
    }
    ESP_LOGI("Doser", "Running program: VTBI=%.2f ml, Flow Rate=%.2f ml/h", vtbi_ml, flow_rate_ml_h);
}

void doser_stop(Doser *doser)
{
    stepper_stop(&doser->motor);
}

void doser_full_open(Doser *doser)
{
    // Move in reverse direction at fast rate to reach open limit
    doser_dispense(doser, -1000.0f, 500.0f, false);
}

void doser_full_close(Doser *doser)
{
    // Move forward at fast rate to reach close limit
    doser_dispense(doser, 1000.0f, 500.0f, false);
}
