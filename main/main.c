#include "main.h"

Doser doser = {
    .motor = {
        .in1 = DRIVER_IN1,
        .in2 = DRIVER_IN2,
        .in3 = DRIVER_IN3,
        .in4 = DRIVER_IN4,
        .number_of_steps = DRIVER_STEPS_PER_REV,
        .step_number = 0,
        .steps_left = 0,
        .last_step_time = 0,
        .step_delay = 1000 / DRIVER_DEFAULT_SPEED,     // in microseconds
        .max_speed_delay = 1000 / DRIVER_DEFAULT_SPEED // in microseconds
    },
    .calibration_factor = DRIVER_STEPS_PER_ML, // steps per ml
};

void act(char *cmd_json, void (*callback)(const char *))
{
    cJSON *cmd = cJSON_Parse(cmd_json);
    if (cmd == NULL)
    {
        ESP_LOGE("ACT", "Failed to parse command JSON");
        return;
    }
    ESP_LOGI("ACT", "Received command: %s", cJSON_Print(cmd));
    if (strcmp(cJSON_GetObjectItem(cmd, "action")->valuestring, "SYNC") == 0)
    {
        callback("ACK");
    }
    else if (strcmp(cJSON_GetObjectItem(cmd, "action")->valuestring, "INSERT_DOSE") == 0)
    {
        callback("ACK");
        float quantity_ml = atof(cJSON_GetObjectItem(cmd, "data")->valuestring);
        float speed_rpm = DRIVER_DEFAULT_SPEED;
        doser_dispense(&doser, quantity_ml, speed_rpm);
        callback("DISPENSED");
    }
    else if (strcmp(cJSON_GetObjectItem(cmd, "action")->valuestring, "STOP") == 0)
    {
        callback("ACK");
        doser_stop(&doser);
    }
    else if (strcmp(cJSON_GetObjectItem(cmd, "action")->valuestring, "RUN_PROGRAM") == 0)
    {
        callback("ACK");
        cJSON *program = cJSON_GetObjectItem(cmd, "data");
        float vtbi = cJSON_GetObjectItem(program, "vtbi")->valuedouble;
        float flow_rate = cJSON_GetObjectItem(program, "flow_rate")->valuedouble;
        doser_run_program(&doser, vtbi, flow_rate);
    }
    else
        ESP_LOGW("ACT", "Unknown command: %s", cJSON_GetObjectItem(cmd, "action")->valuestring);
}

void initialize_all(void)
{
    doser_init(&doser, DRIVER_STEPS_PER_REV, DRIVER_DEFAULT_SPEED, doser.calibration_factor);
    lcd_init();
    io_module_init();
}

void start_modules(void)
{
    start_bt_module();
}

void app_main(void)
{
    initialize_all();
    start_modules();
}
