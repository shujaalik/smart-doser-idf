#include "storage.h"

extern program_t programs[10];
extern int program_count;

void init_spiffs(void)
{
    char *TAG = "SPIFFS_INITALIZATION";
    nvs_flash_init();
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/storage",
        .partition_label = "storage",
        .max_files = 5,
        .format_if_mount_failed = true,
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        else if (ret == ESP_ERR_NOT_FOUND)
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        else
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        return;
    }
    fetch_programs();
}

esp_err_t available_storage_fs(void)
{
    size_t total = 0, used = 0;
    esp_err_t ret = esp_spiffs_info("storage", &total, &used);
    float used_percent = (float)used / (float)total * 100;
    if (ret != ESP_OK)
        printf("Failed to get SPIFFS partition information (%s)\n", esp_err_to_name(ret));
    else
    {
        printf("Partition size: total: %d, used: %d\n", total, used);
        printf("Partition used: %.2f%%\n", used_percent);
    }
    if (esp_get_free_heap_size() < 60000)
    {
        esp_restart();
    }
    if (used_percent > 80)
        return ESP_FAIL;
    return ret;
}
float get_storage_used_percent(void)
{
    size_t total = 0, used = 0;
    esp_err_t ret = esp_spiffs_info("storage", &total, &used);
    if (ret != ESP_OK)
        printf("Failed to get SPIFFS partition information (%s)\n", esp_err_to_name(ret));
    else
    {
        float used_percent = (float)used / (float)total * 100;
        return used_percent;
    }
    return -1.0f; // Error case
}

void save_log(cJSON *log)
{
    char *TAG = "SAVE_LOG";
    if (available_storage_fs() != ESP_OK)
    {
        ESP_LOGE(TAG, "Storage is full, cannot save log");
        return;
    }
    char *log_str = cJSON_PrintUnformatted(log);
    if (log_str == NULL)
    {
        ESP_LOGE(TAG, "Failed to print log to string");
        return;
    }
    time_t timestamp = (time_t)cJSON_GetObjectItem(log, "timestamp")->valueint;
    char filename[64];
    struct tm *timeinfo = localtime(&timestamp);
    snprintf(filename, sizeof(filename), "/storage/logs/log_%04d%02d%02d_%02d%02d%02d.txt",
             timeinfo->tm_year + 1900,
             timeinfo->tm_mon + 1,
             timeinfo->tm_mday,
             timeinfo->tm_hour,
             timeinfo->tm_min,
             timeinfo->tm_sec);
    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", filename);
        free(log_str);
        return;
    }
    fprintf(file, "%s", log_str);
    fclose(file);
    free(log_str);
}

cJSON *read_log(void)
{
    // return the first file content from /storage/logs
    char *TAG = "READ_LOG";
    if (available_storage_fs() != ESP_OK)
    {
        ESP_LOGE(TAG, "Storage is full, cannot read log");
        return NULL;
    }
    DIR *dir = opendir("/storage/logs");
    if (dir == NULL)
    {
        ESP_LOGE(TAG, "Failed to open storage directory");
        return NULL;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_REG)
        {
            char filepath[128];
            snprintf(filepath, sizeof(filepath), "/storage/logs/%.100s", entry->d_name);
            FILE *file = fopen(filepath, "r");
            if (file == NULL)
            {
                ESP_LOGE(TAG, "Failed to open log file for reading: %s", filepath);
                continue;
            }
            fseek(file, 0, SEEK_END);
            long filesize = ftell(file);
            fseek(file, 0, SEEK_SET);
            char *filecontent = malloc(filesize + 1);
            if (filecontent == NULL)
            {
                ESP_LOGE(TAG, "Failed to allocate memory for file content");
                fclose(file);
                continue;
            }
            fread(filecontent, 1, filesize, file);
            fclose(file);
            filecontent[filesize] = '\0';
            cJSON *log = cJSON_Parse(filecontent);
            free(filecontent);
            if (log == NULL)
            {
                ESP_LOGE(TAG, "Failed to parse log file: %s", filepath);
                continue;
            }
            closedir(dir);
            return log;
        }
    }
    closedir(dir);
    return NULL;
}

void delete_log(void)
{
    char *TAG = "DELETE_LOG";
    if (available_storage_fs() != ESP_OK)
    {
        ESP_LOGE(TAG, "Storage is full, cannot delete log");
        return;
    }
    DIR *dir = opendir("/storage/logs");
    if (dir == NULL)
    {
        ESP_LOGE(TAG, "Failed to open storage directory");
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_REG)
        {
            char filepath[128];
            snprintf(filepath, sizeof(filepath), "/storage/logs/%.100s", entry->d_name);
            if (remove(filepath) != 0)
            {
                ESP_LOGE(TAG, "Failed to delete log file: %s", filepath);
            }
            else
            {
                ESP_LOGI(TAG, "Deleted log file: %s", filepath);
            }
        }
    }
    closedir(dir);
}

void fetch_programs(void)
{
    char *TAG = "FETCH_PROGRAMS";
    if (available_storage_fs() != ESP_OK)
    {
        ESP_LOGE(TAG, "Storage is full, cannot fetch programs");
        return;
    }
    DIR *dir = opendir("/storage/programs");
    if (dir == NULL)
    {
        ESP_LOGE(TAG, "Failed to open storage directory");
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_REG)
        {
            char filepath[128];
            snprintf(filepath, sizeof(filepath), "/storage/programs/%.100s", entry->d_name);
            FILE *file = fopen(filepath, "r");
            if (file == NULL)
            {
                ESP_LOGE(TAG, "Failed to open program file for reading: %s", filepath);
                continue;
            }
            fseek(file, 0, SEEK_END);
            long filesize = ftell(file);
            fseek(file, 0, SEEK_SET);
            char *filecontent = malloc(filesize + 1);
            if (filecontent == NULL)
            {
                ESP_LOGE(TAG, "Failed to allocate memory for file content");
                fclose(file);
                continue;
            }
            fread(filecontent, 1, filesize, file);
            fclose(file);
            filecontent[filesize] = '\0';
            cJSON *program = cJSON_Parse(filecontent);
            free(filecontent);
            if (program == NULL)
            {
                ESP_LOGE(TAG, "Failed to parse program file: %s", filepath);
                continue;
            }
            if (program_count < 10)
            {
                programs[program_count++] = (program_t){
                    .vtbi = cJSON_GetObjectItem(program, "vtbi")->valuedouble,
                    .flow_rate = cJSON_GetObjectItem(program, "flow_rate")->valuedouble,
                    .name = cJSON_GetObjectItem(program, "name")->valuestring};
            }
            else
            {
                ESP_LOGW(TAG, "Program storage full, cannot fetch more programs");
            }
            cJSON_Delete(program);
        }
    }
    closedir(dir);
}

void save_programs(void)
{
    char *TAG = "SAVE_PROGRAMS";
    if (available_storage_fs() != ESP_OK)
    {
        ESP_LOGE(TAG, "Storage is full, cannot save programs");
        return;
    }
    for (int i = 0; i < program_count; i++)
    {
        char filepath[128];
        snprintf(filepath, sizeof(filepath), "/storage/programs/program_%d.json", i);
        FILE *file = fopen(filepath, "w");
        if (file == NULL)
        {
            ESP_LOGE(TAG, "Failed to open program file for writing: %s", filepath);
            continue;
        }
        cJSON *program = cJSON_CreateObject();
        cJSON_AddNumberToObject(program, "vtbi", programs[i].vtbi);
        cJSON_AddNumberToObject(program, "flow_rate", programs[i].flow_rate);
        cJSON_AddStringToObject(program, "name", programs[i].name);
        char *json = cJSON_Print(program);
        if (json == NULL)
        {
            ESP_LOGE(TAG, "Failed to print program JSON: %s", filepath);
            fclose(file);
            continue;
        }
        fwrite(json, 1, strlen(json), file);
        fclose(file);
        free(json);
    }
}
