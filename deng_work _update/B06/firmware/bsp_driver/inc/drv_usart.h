/*
 * File      : usart.h
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2009, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      the first version
 */

#ifndef __USART_H__
#define __USART_H__

#include <rthw.h>
#include <rtthread.h>
#include "stm32f4xx_hal_uart.h"

#define UART_ENABLE_IRQ(n)            HAL_NVIC_EnableIRQ((n))
#define UART_DISABLE_IRQ(n)           HAL_NVIC_DisableIRQ((n))

/* STM32 uart driver */
struct stm32_uart
{
    USART_TypeDef* uart_device;
    IRQn_Type irq;
    rt_uint8_t irq_pre_pri;
    rt_uint8_t irq_sub_pri;
    DMA_Stream_TypeDef* tx_stream_channel;
    DMA_Stream_TypeDef* rx_stream_channel;
    IRQn_Type dma_tx_irq;
    IRQn_Type dma_rx_irq;
	uint32_t tx_dma_channel;
	uint32_t rx_dma_channel;
	UART_HandleTypeDef UartHandle;
	DMA_HandleTypeDef  UARTTxDMA_Handler;
	DMA_HandleTypeDef  UARTRxDMA_Handler;
	void *user_data;
};

void test_send_uartdma(void);

int stm32_hw_usart_init(void);
void MX_USART1_UART_Init(void);
#endif
