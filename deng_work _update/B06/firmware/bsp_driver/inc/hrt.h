#ifndef __HRT_H_
#define __HRT_H_

#include "stdint.h"
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"

int hrt_init(void);
uint64_t hrt_absolute_time(void);
uint64_t hrt_absolute_ms(void);
void hrt_delay_us(uint32_t us);
void hrt_delay_ms(uint32_t ms);

#endif



