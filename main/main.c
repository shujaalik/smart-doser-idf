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
        .step_delay = 1000 / DRIVER_DEFAULT_SPEED,     // in microseconds
        .max_speed_delay = 1000 / DRIVER_DEFAULT_SPEED // in microseconds
    },
    .calibration_factor = DRIVER_STEPS_PER_ML, // steps per ml
};
program_t programs[10];
int program_count;

void act(char *cmd_json, void (*callback)(const char *))
{
    cJSON *cmd = cJSON_Parse(cmd_json);
    if (cmd == NULL)
    {
        ESP_LOGE("ACT", "Failed to parse command JSON");
        return;
    }
    ESP_LOGI("ACT", "Received command: %s", cJSON_Print(cmd));
    if (strcmp(cJSON_GetObjectItem(cmd, "action")->valuestring, "SCAN") == 0)
    {
        callback("SCAN_ACK");
    }
    else if (strcmp(cJSON_GetObjectItem(cmd, "action")->valuestring, "GET_MAC") == 0)
    {
        char *mac = mac_address();
        callback(mac);
        free(mac);
    }
    else if (strcmp(cJSON_GetObjectItem(cmd, "action")->valuestring, "SYNC") == 0)
    {
        callback("ACK");
    }
    else if (strcmp(cJSON_GetObjectItem(cmd, "action")->valuestring, "INSERT_DOSE") == 0)
    {
        callback("ACK");
        // data in float
        float quantity_ml = cJSON_GetObjectItem(cmd, "data")->valuedouble;
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
        doser_run_program(&doser, vtbi, flow_rate, true);
    }
    else if (strcmp(cJSON_GetObjectItem(cmd, "action")->valuestring, "SAVE_PROGRAM") == 0)
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
    else if (strcmp(cJSON_GetObjectItem(cmd, "action")->valuestring, "FULL_OPEN") == 0)
    {
        callback("ACK");
        doser_full_open(&doser);
        ESP_LOGI("ACT", "Stepper motor moved to full open position");
    }
    else if (strcmp(cJSON_GetObjectItem(cmd, "action")->valuestring, "FULL_CLOSED") == 0)
    {
        callback("ACK");
        doser_full_close(&doser);
        ESP_LOGI("ACT", "Stepper motor moved to full closed position");
    }
    else if (strcmp(cJSON_GetObjectItem(cmd, "action")->valuestring, "RTC_SET") == 0)
    {
        ESP_LOGI("ACT", "RTC_SET command received");
        cJSON *data = cJSON_GetObjectItem(cmd, "data");
        struct tm time = {
            .tm_year = 100, // since 1900 (2016 - 1900)
            .tm_mon = 0,    // 0-based
            .tm_mday = 0,
            .tm_hour = 0,
            .tm_min = 0,
            .tm_sec = 0};
        cJSON *year = cJSON_GetObjectItemCaseSensitive(data, "year");
        cJSON *month = cJSON_GetObjectItemCaseSensitive(data, "month");
        cJSON *date = cJSON_GetObjectItemCaseSensitive(data, "date");
        cJSON *hour = cJSON_GetObjectItemCaseSensitive(data, "hour");
        cJSON *minute = cJSON_GetObjectItemCaseSensitive(data, "minute");
        cJSON *second = cJSON_GetObjectItemCaseSensitive(data, "second");
        if (cJSON_IsNumber(year) && (year->valueint != 0))
            time.tm_year = year->valueint - 1900;
        if (cJSON_IsNumber(month) && (month->valueint != 0))
            time.tm_mon = month->valueint - 1;
        if (cJSON_IsNumber(date) && (date->valueint != 0))
            time.tm_mday = date->valueint;
        if (cJSON_IsNumber(hour) && (hour->valueint != 0))
            time.tm_hour = hour->valueint;
        if (cJSON_IsNumber(minute) && (minute->valueint != 0))
            time.tm_min = minute->valueint;
        if (cJSON_IsNumber(second) && (second->valueint != 0))
            time.tm_sec = second->valueint;
        printf("Setting RTC to %d-%d-%d %d:%d:%d\n", time.tm_year, time.tm_mon, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
        set_time(&time);
        callback("RTC_SET_DONE");
        ESP_LOGI("ACT", "RTC_SET DONE");
    }
    else
        ESP_LOGW("ACT", "Unknown command: %s", cJSON_GetObjectItem(cmd, "action")->valuestring);
}

void initialize_all(void)
{
    init_spiffs();
    time_manager_init();
    doser_init(&doser, DRIVER_STEPS_PER_REV, DRIVER_DEFAULT_SPEED, doser.calibration_factor);
    lcd_init();
    doser_full_open(&doser); // Move to full open position initially
    io_module_init();
    init_wifi_module();
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
