#ifndef __BSP_LED_H
#define __BSP_LED_H

#include "stm32f4xx_hal.h"

void power_led_enable(void);
void power_led_disable(void);
void power_led_init(void);

void indicator_led_enable(void);
void indicator_led_disable(void);
void indicator_led_init(void);

void status_led_enable(void);
void status_led_disable(void);
void status_led_init(void);
#endif
