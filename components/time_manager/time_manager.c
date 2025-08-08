#include "time_manager.h"

i2c_dev_t dev;

void time_manager_init(void)
{
    esp_err_t err = i2cdev_init();
    if (err != ESP_OK)
    {
        ESP_LOGE("TIME_MANAGER", "Failed to initialize I2C device: %s", esp_err_to_name(err));
        return;
    }
    memset(&dev, 0, sizeof(i2c_dev_t));
    err = ds3231_init_desc(&dev, I2C_NUM_0, GPIO_NUM_21, GPIO_NUM_22);
    if (err != ESP_OK)
    {
        ESP_LOGE("TIME_MANAGER", "Failed to initialize DS3231 descriptor: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI("TIME_MANAGER", "Time manager initialized successfully");
    struct tm time;
    struct tm current_time = get_time();
    ESP_LOGI("TIME_MANAGER", "Current time: %04d-%02d-%02d %02d:%02d:%02d",
             current_time.tm_year, current_time.tm_mon, current_time.tm_mday,
             current_time.tm_hour, current_time.tm_min, current_time.tm_sec);
}

struct tm get_time(void)
{
    struct tm time;
    ds3231_get_time(&dev, &time);
    time.tm_year += 1900; // Adjust year to match struct tm format
    time.tm_mon += 1;     // Adjust month to match struct tm format
    time.tm_isdst = -1;   // Let the system determine if DST is in
    return time;
}

void set_time(struct tm *time)
{
    if (time == NULL)
    {
        ESP_LOGE("TIME_MANAGER", "Invalid time structure provided");
        return;
    }
    time->tm_year -= 1900; // Adjust year for DS3231
    time->tm_mon -= 1;     // Adjust month for DS3231
    ds3231_set_time(&dev, time);
    ESP_LOGI("TIME_MANAGER", "Time set to: %04d-%02d-%02d %02d:%02d:%02d",
             time->tm_year + 1900, time->tm_mon + 1, time->tm_mday,
             time->tm_hour, time->tm_min, time->tm_sec);
}
