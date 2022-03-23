#ifndef __USART_H__
#define __USART_H__

#include "stm32f4xx_hal_uart.h"
#include "stdlib.h"
#include "stdio.h"
#include "stdarg.h"

void stm32_uart_init(void);
int fputc(int ch, FILE *f);



#endif

 

