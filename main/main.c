#include "main.h"

Doser doser = {
    .motor = {
        .in1 = DRIVER_IN1,
        .in2 = DRIVER_IN2,
        .in3 = DRIVER_IN3,
        .in4 = DRIVER_IN4,
        .full_closed_switch = FULL_CLOSED_SWITCH,
        .full_open_switch = FULL_OPEN_SWITCH,
        .number_of_steps = DRIVER_STEPS_PER_REV,
        .step_number = 0,
        .steps_left = 0,
        .last_step_time = 0,
        .step_delay = 0, // will be set based on flow rate
        .max_speed_delay = 0},
    .calibration_factor = DRIVER_STEPS_PER_ML, // steps per ml
};

program_t programs[10];
int program_count;
bool wifi_connected = false;
bool mqtt_connected = false;
bool bt_connected = false;

void act(char *cmd_json, void (*callback)(const char *))
{
    cJSON *cmd = cJSON_Parse(cmd_json);
    if (cmd == NULL)
    {
        ESP_LOGE("ACT", "Failed to parse command JSON");
        return;
    }
    ESP_LOGI("ACT", "Received command: %s", cJSON_Print(cmd));

    const char *action = cJSON_GetObjectItem(cmd, "action")->valuestring;

    if (strcmp(action, "SCAN") == 0)
    {
        callback("SCAN_ACK");
    }
    else if (strcmp(action, "GET_MAC") == 0)
    {
        char *mac = mac_address();
        callback(mac);
        free(mac);
    }
    else if (strcmp(action, "SYNC") == 0)
    {
        callback("ACK");
    }
    else if (strcmp(action, "INSERT_DOSE") == 0)
    {
        callback("ACK");
        float quantity_ml = cJSON_GetObjectItem(cmd, "data")->valuedouble;
        float flow_rate_ml_h = DRIVER_DEFAULT_FLOW_RATE; // ml/h instead of RPM
        doser_dispense(&doser, quantity_ml, flow_rate_ml_h, true);
        callback("DISPENSED");
    }
    else if (strcmp(action, "STOP") == 0)
    {
        callback("ACK");
        doser_stop(&doser);
    }
    else if (strcmp(action, "RUN_PROGRAM") == 0)
    {
        callback("ACK");
        cJSON *program = cJSON_GetObjectItem(cmd, "data");
        float vtbi = cJSON_GetObjectItem(program, "vtbi")->valuedouble;
        float flow_rate = cJSON_GetObjectItem(program, "flow_rate")->valuedouble; // already ml/h
        doser_run_program(&doser, vtbi, flow_rate, true);
    }
    else if (strcmp(action, "SAVE_PROGRAM") == 0)
    {
        callback("ACK");
        cJSON *program = cJSON_GetObjectItem(cmd, "data");
        float vtbi = cJSON_GetObjectItem(program, "vtbi")->valuedouble;
        float flow_rate = cJSON_GetObjectItem(program, "flow_rate")->valuedouble;
        program_t new_program = {
            .vtbi = vtbi,
            .flow_rate = flow_rate,
            .name = cJSON_GetObjectItem(program, "name")->valuestring};
        programs[program_count++] = new_program;
        ESP_LOGI("ACT", "Program saved: %s", new_program.name);
        save_programs();
    }
    else if (strcmp(action, "FULL_OPEN") == 0)
    {
        callback("ACK");
        doser_full_open(&doser);
        ESP_LOGI("ACT", "Stepper motor moved to full open position");
    }
    else if (strcmp(action, "FULL_CLOSED") == 0)
    {
        callback("ACK");
        doser_full_close(&doser);
        ESP_LOGI("ACT", "Stepper motor moved to full closed position");
    }
    else if (strcmp(action, "RTC_SET") == 0)
    {
        ESP_LOGI("ACT", "RTC_SET command received");
        cJSON *data = cJSON_GetObjectItem(cmd, "data");
        struct tm time = {0};
        cJSON *year = cJSON_GetObjectItemCaseSensitive(data, "year");
        cJSON *month = cJSON_GetObjectItemCaseSensitive(data, "month");
        cJSON *date = cJSON_GetObjectItemCaseSensitive(data, "date");
        cJSON *hour = cJSON_GetObjectItemCaseSensitive(data, "hour");
        cJSON *minute = cJSON_GetObjectItemCaseSensitive(data, "minute");
        cJSON *second = cJSON_GetObjectItemCaseSensitive(data, "second");

        if (cJSON_IsNumber(year))
            time.tm_year = year->valueint - 1900;
        if (cJSON_IsNumber(month))
            time.tm_mon = month->valueint - 1;
        if (cJSON_IsNumber(date))
            time.tm_mday = date->valueint;
        if (cJSON_IsNumber(hour))
            time.tm_hour = hour->valueint;
        if (cJSON_IsNumber(minute))
            time.tm_min = minute->valueint;
        if (cJSON_IsNumber(second))
            time.tm_sec = second->valueint;

        printf("Setting RTC to %d-%d-%d %d:%d:%d\n",
               time.tm_year + 1900, time.tm_mon + 1, time.tm_mday,
               time.tm_hour, time.tm_min, time.tm_sec);

        set_time(&time);
        callback("RTC_SET_DONE");
        ESP_LOGI("ACT", "RTC_SET DONE");
    }
    else
    {
        ESP_LOGW("ACT", "Unknown command: %s", action);
    }
}

void initialize_all(void)
{
    init_spiffs();
    time_manager_init();
    lcd_init();
    doser_init(&doser, DRIVER_STEPS_PER_REV, doser.calibration_factor); // no RPM param
    io_module_init();
    init_wifi_module();
    main_screen();
}

void start_modules(void)
{
    start_bt_module();
    start_wifi_module();
}

void app_main(void)
{
    initialize_all();
    start_modules();
}
