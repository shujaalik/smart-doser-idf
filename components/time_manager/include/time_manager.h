#pragma once
#ifndef _TIME_MANAGER_H_
#define _TIME_MANAGER_H_

#include "main.h"
#include <ds3231.h>
#include "i2cdev.h"

void time_manager_init(void);
void set_time(struct tm *time);
struct tm get_time(void);

#endif