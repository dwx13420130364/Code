#ifndef __DRIVER_KEY_H
#define __DRIVER_KEY_H

#include "stm32f4xx_hal.h"

uint8_t wifi_reset_button_read_status(void);
void wifi_reset_button_init(void);
uint8_t wifi_sta_button_read_status(void);
void wifi_sta_button_init(void);
void uwb_power_out_enable(void);
void uwb_power_out_disable(void);
void uwb_power_out_init(void);
void wifi_power_out_enable(void);
void wifi_power_out_disable(void);
void wifi_power_out_init(void);
#endif
