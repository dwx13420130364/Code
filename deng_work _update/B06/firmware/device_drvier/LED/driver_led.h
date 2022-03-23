#ifndef __DRIVER_LED_H
#define __DRIVER_LED_H

#include "stm32f4xx_hal.h"

void battery_led_disable(void);
void battery_led_enable(void);
void battery_led_init(void);




void indicator_led_enable(void);
void indicator_led_disable(void);
void indicator_led_init(void);


#endif
