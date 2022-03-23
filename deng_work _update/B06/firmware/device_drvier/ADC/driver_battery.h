#ifndef __DRIVER_BATTERY_H
#define __DRIVER_BATTERY_H

#include "stm32f4xx_hal.h"
#include "rtthread.h"
#include "stdbool.h"

uint16_t bat_get_adc_value(void);
void bat_adc_init(void);

#endif
