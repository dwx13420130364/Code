/*
 * File      : board.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2015, RT-Thread Development Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      first implementation
 */

#include <rtthread.h>
#include "board.h"

#include "finsh.h"


void _Error_Handler(char *file, int line)
{
  while(1)
  {
  }
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 200000000
  *            HCLK(Hz)                       = 200000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 25
  *            PLL_N                          = 400
  *            PLL_P                          = 2
  *            PLLSAI_N                       = 384
  *            PLLSAI_P                       = 8
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 6
  *			Fvco=Fs*(plln/pllm);
  *			Fsys=Fvco/pllp=Fs*(plln/(pllm*pllp));
  *			Fusb=Fvco/pllq=Fs*(plln/(pllm*pllq));
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_OscInitTypeDef RCC_OscInitStruct;
    HAL_StatusTypeDef ret = HAL_OK;

	/* Enable Power Control clock */
	__HAL_RCC_PWR_CLK_ENABLE();
	
    /* Enable HSE Oscillator and activate PLL with HSE as source */
	RCC_OscInitStruct.OscillatorType =  RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.LSIState = RCC_LSI_ON;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 336;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
	RCC_OscInitStruct.PLL.PLLQ = 7;

    ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
    if(ret != HAL_OK)
    {
        while (1) { ; }
    }

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
       clocks dividers */
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |\
                                   RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
    if (ret != HAL_OK)
    {
        while (1) { ; }
    }
}


/**
 * This is the timer interrupt service routine.
 *
 */
void SysTick_Handler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    /* tick for HAL Library */
    HAL_IncTick();

    rt_tick_increase();

    /* leave interrupt */
    rt_interrupt_leave();
}

/* re-implementat tick interface for STM32 HAL */
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    /*Configure the SysTick to have interrupt in 1ms time basis*/
    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/RT_TICK_PER_SECOND);

    /*Configure the SysTick IRQ priority */
    HAL_NVIC_SetPriority(SysTick_IRQn, TickPriority ,0);

    /* Return function status */
    return HAL_OK;
}

void HAL_Delay(__IO uint32_t Delay)
{
    rt_thread_delay(Delay);
}

#include "drv_usart.h"
extern void hg_show_version(void);

void rt_hw_board_init(void)
{	
    HAL_Init();
    /* Configure the system clock @ 200 Mhz */
    SystemClock_Config();
    /* init systick */
    SysTick_Config(SystemCoreClock / RT_TICK_PER_SECOND);
    /* set pend exception priority */
    NVIC_SetPriority(PendSV_IRQn, (1 << __NVIC_PRIO_BITS) - 1);

#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif
	
    rt_system_heap_init((void*)HEAP_BEGIN, (void*)HEAP_END);
	//??????????
	stm32_hw_usart_init();
//	xbee_sta_button_init();
//	wifi_sta_button_init();
#ifdef RT_USING_CONSOLE
	rt_console_set_device("uart2");
#endif
	rt_show_version();
}
