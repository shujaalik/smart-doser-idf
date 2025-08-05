#pragma once
#ifndef _LCD1604_H_
#define _LCD1604_H_

#include "main.h"
#include "driver/i2c.h"
#include <unistd.h>

void lcd_init(void); // initialize lcd

void lcd_send_cmd(char cmd); // send command to the lcd

void lcd_send_data(char data); // send data to the lcd

void lcd_send_string(char *str); // send string to the lcd

void lcd_put_cur(int row, int col); // put cursor at the entered position row (0 or 1), col (0-15);

void lcd_blink_cursor(void);
void lcd_clear(void);

void initializing_screen(void);
void main_menu_screen(main_select_t selected_mode);
void manual_control_screen(manual_control_t manual_control_data, manual_control_cursor_t manual_control_cursor);
void scheduled_dose_screen(void);

#endif