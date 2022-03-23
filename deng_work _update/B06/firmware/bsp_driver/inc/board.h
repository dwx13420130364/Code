#ifndef __BOARD_H__
#define __BOARD_H__

#include <stm32f4xx.h>
#include "stm32f4xx_hal.h"

//定义内存分配的起始位置
#ifdef __CC_ARM
extern int Image$$RW_IRAM1$$ZI$$Limit;
#define HEAP_BEGIN    (&Image$$RW_IRAM1$$ZI$$Limit)
#elif __ICCARM__
#pragma section="HEAP"
#define HEAP_BEGIN    (__segment_end("HEAP"))
#else
extern int __bss_end;
#define HEAP_BEGIN    (&__bss_end)
#endif

//定义内存分配结束位置
#define STM32_SRAM_SIZE   (64 * 1024)
#define HEAP_END          (0x20000000 + STM32_SRAM_SIZE)

void rt_hw_board_init(void);

//使用串口作为控制台输出
#define STM32_CONSOLE_USART		2

#if STM32_CONSOLE_USART == 0
#define CONSOLE_DEVICE "no"
#elif STM32_CONSOLE_USART == 1
#define CONSOLE_DEVICE "uart1"
#elif STM32_CONSOLE_USART == 2
#define CONSOLE_DEVICE "uart2"
#elif STM32_CONSOLE_USART == 3
#define CONSOLE_DEVICE "uart3"
#elif STM32_CONSOLE_USART == 6
#define CONSOLE_DEVICE "uart6"
#endif







#define FINSH_DEVICE_NAME   CONSOLE_DEVICE
#endif
