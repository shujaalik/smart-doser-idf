#include "lcd1604.h"

esp_err_t err;

static const char *TAG = "LCD1604";

static esp_err_t i2c_master_init(void)
{
    return ESP_OK;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_NUM_21,
        .scl_io_num = GPIO_NUM_22,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };

    i2c_param_config(I2C_NUM_0, &conf);

    return i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
}

void lcd_send_cmd(char cmd)
{
    char data_u, data_l;
    uint8_t data_t[4];
    data_u = (cmd & 0xf0);
    data_l = ((cmd << 4) & 0xf0);
    data_t[0] = data_u | 0x0C; // en=1, rs=0
    data_t[1] = data_u | 0x08; // en=0, rs=0
    data_t[2] = data_l | 0x0C; // en=1, rs=0
    data_t[3] = data_l | 0x08; // en=0, rs=0
    err = i2c_master_write_to_device(I2C_NUM_0, LCD_ADDR, data_t, 4, 1000);
    if (err != 0)
        ESP_LOGI(TAG, "Error in sending command");
}

void lcd_send_data(char data)
{
    char data_u, data_l;
    uint8_t data_t[4];
    data_u = (data & 0xf0);
    data_l = ((data << 4) & 0xf0);
    data_t[0] = data_u | 0x0D; // en=1, rs=0
    data_t[1] = data_u | 0x09; // en=0, rs=0
    data_t[2] = data_l | 0x0D; // en=1, rs=0
    data_t[3] = data_l | 0x09; // en=0, rs=0
    err = i2c_master_write_to_device(I2C_NUM_0, LCD_ADDR, data_t, 4, 1000);
    if (err != 0)
        ESP_LOGI(TAG, "Error in sending data");
}

void lcd_blink_cursor(void)
{
    lcd_send_cmd(0x0F); // Display on, cursor on, blink on
    usleep(1000);
}

void lcd_clear(void)
{
    // stop blinking cursor
    lcd_send_cmd(0x0C);
    usleep(1000);
    lcd_send_cmd(0x01);
    usleep(5000);
}

void lcd_put_cur(int row, int col)
{
    switch (row)
    {
    case 0:
        col |= 0x80;
        break;
    case 1:
        col |= 0xC0;
        break;
    case 2:
        col |= 0x90;
        break;
    case 3:
        col |= 0xD0;
        break;
    default:
        ESP_LOGI(TAG, "Invalid row number. Use 0-3.");
        return;
    }

    lcd_send_cmd(col);
}

void initializing_screen(void)
{
    lcd_put_cur(0, 0);
    lcd_send_string("Iqra University");
    lcd_put_cur(1, 0);
    lcd_send_string("Influsion Pump");
    lcd_put_cur(2, 0);
    lcd_send_string("Initializing...");
}

void main_menu_screen(main_select_t selected_mode)
{
    lcd_clear();
    lcd_put_cur(0, 0);
    lcd_send_string("Select Mode:");
    lcd_put_cur(1, 0);
    lcd_send_string(selected_mode == 0 ? "> Manual Control" : "  Manual Control");
    lcd_put_cur(2, 0);
    lcd_send_string(selected_mode == 1 ? "> Scheduled Dose" : "  Scheduled Dose");
    lcd_put_cur(3, 0);
    lcd_send_string(selected_mode == 2 ? "> Purge/Unpurge" : "  Purge/Unpurge");
}

void manual_control_screen(manual_control_t manual_control_data, manual_control_cursor_t manual_control_cursor)
{
    float flow = manual_control_data.flow_rate;
    float volume = manual_control_data.volume;
    lcd_clear();
    lcd_put_cur(0, 0);
    lcd_send_string("Manual Control");
    lcd_put_cur(1, 0);
    lcd_send_string("FLW: ");
    lcd_put_cur(1, 5);
    char flow_str[10];
    snprintf(flow_str, sizeof(flow_str), "%.2f ml/h", flow);
    lcd_send_string(flow_str);
    lcd_put_cur(2, 0);
    lcd_send_string("VOL: ");
    lcd_put_cur(2, 5);
    char vol_str[10];
    snprintf(vol_str, sizeof(vol_str), "%.2f ml", volume);
    lcd_send_string(vol_str);
    lcd_put_cur(3, 0);
    lcd_send_string("PAU/RES:* HOME:*");
    if (manual_control_cursor == CURSOR_FLOW_RATE)
        lcd_put_cur(1, 5);
    else if (manual_control_cursor == CURSOR_VOLUME)
        lcd_put_cur(2, 5);
    else if (manual_control_cursor == CURSOR_START_STOP)
        lcd_put_cur(3, 8);
    else if (manual_control_cursor == CURSOR_HOME)
        lcd_put_cur(3, 15);
    lcd_blink_cursor();
}

void scheduled_dose_screen(void)
{
    lcd_clear();
    lcd_put_cur(0, 0);
    lcd_send_string("VTBI: 5.0 ml");
    lcd_put_cur(1, 0);
    lcd_send_string("FLOW: 100 ml/h");
    lcd_put_cur(2, 0);
    lcd_send_string("TIME: 00:00:00");
    lcd_put_cur(3, 0);
    lcd_send_string("PAU/RES:* HOME:*");
}

void lcd_init(void)
{
    err = i2c_master_init();
    if (err != 0)
    {
        ESP_LOGI(TAG, "Error in I2C master initialization");
        return;
    }
    // 4 bit initialisation
    usleep(50000); // wait for >40ms
    lcd_send_cmd(0x30);
    usleep(5000); // wait for >4.1ms
    lcd_send_cmd(0x30);
    usleep(200); // wait for >100us
    lcd_send_cmd(0x30);
    usleep(10000);
    lcd_send_cmd(0x20); // 4bit mode
    usleep(10000);

    // dislay initialisation
    lcd_send_cmd(0x28); // Function set --> DL=0 (4 bit mode), N = 1 (2 line display) F = 0 (5x8 characters)
    usleep(1000);
    lcd_send_cmd(0x08); // Display on/off control --> D=0,C=0, B=0  ---> display off
    usleep(1000);
    lcd_send_cmd(0x01); // clear display
    usleep(1000);
    usleep(1000);
    lcd_send_cmd(0x06); // Entry mode set --> I/D = 1 (increment cursor) & S = 0 (no shift)
    usleep(1000);
    lcd_send_cmd(0x0C); // Display on/off control --> D = 1, C and B = 0. (Cursor and blink, last two bits)
    usleep(1000);
    lcd_clear(); // clear display
    initializing_screen();
}

void lcd_send_string(char *str)
{
    while (*str)
        lcd_send_data(*str++);
}