#include "hrt.h"
#include "rthw.h"
#include "rtthread.h"

#define HRT_TIMER					TIM4
#define HRT_TIMER_CLK_ENABLE()		__HAL_RCC_TIM4_CLK_ENABLE()
#define HRT_TIMER_IRQn				TIM4_IRQn
#define HRT_TIMER_IRQHandler		TIM4_IRQHandler

#define HRT_COUNTER_SCALE(_c)	(_c)
#define PERIOD_HRT_TIMER    	50000

TIM_HandleTypeDef HRT_TIMER_Handler;      	
static  uint64_t Hrt_Time_Accumulated = 0;	
static  uint32_t timer_count_last = 0;		

int hrt_init(void)
{
	HRT_TIMER_Handler.Instance=HRT_TIMER;
    HRT_TIMER_Handler.Init.Prescaler=83;
    HRT_TIMER_Handler.Init.CounterMode=TIM_COUNTERMODE_UP;
    HRT_TIMER_Handler.Init.Period=PERIOD_HRT_TIMER - 1;
    HRT_TIMER_Handler.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_Base_Init(&HRT_TIMER_Handler);
    
    HAL_TIM_Base_Start_IT(&HRT_TIMER_Handler);
	__HAL_TIM_CLEAR_FLAG(&HRT_TIMER_Handler, TIM_FLAG_UPDATE);
	return 0;
}
INIT_BOARD_EXPORT(hrt_init);

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
	/* 使能定时器时钟，设置定时器中断优先级，使能定时器中断 */
	HRT_TIMER_CLK_ENABLE();            			
	HAL_NVIC_SetPriority(HRT_TIMER_IRQn,0,0);   
	HAL_NVIC_EnableIRQ(HRT_TIMER_IRQn);       
}

void HRT_TIMER_IRQHandler(void)
{
	/* enter interrupt */
    rt_interrupt_enter();
	if(__HAL_TIM_GET_FLAG(&HRT_TIMER_Handler, TIM_FLAG_UPDATE) != RESET)
	{
		if(__HAL_TIM_GET_IT_SOURCE(&HRT_TIMER_Handler, TIM_IT_UPDATE) !=RESET)
		{
			/* timer_count_last的值为0 */
			timer_count_last = HRT_TIMER->CNT;
			Hrt_Time_Accumulated = HRT_COUNTER_SCALE(Hrt_Time_Accumulated + PERIOD_HRT_TIMER);
		}
		__HAL_TIM_CLEAR_IT(&HRT_TIMER_Handler, TIM_IT_UPDATE);
	}
    /* leave interrupt */
    rt_interrupt_leave();

}

uint64_t hrt_absolute_time(void)
{
	register rt_base_t level;
    uint32_t timer_count = 0;
    uint64_t Hrt_Time_now = 0;

    /* disable interrupt */
    level = rt_hw_interrupt_disable();
    timer_count = HRT_TIMER->CNT;
    if((timer_count < timer_count_last))
    {
        Hrt_Time_Accumulated += PERIOD_HRT_TIMER;
		__HAL_TIM_CLEAR_FLAG(&HRT_TIMER_Handler, TIM_FLAG_UPDATE);
		__HAL_TIM_CLEAR_IT(&HRT_TIMER_Handler, TIM_IT_UPDATE);
    }
    Hrt_Time_now = HRT_COUNTER_SCALE(Hrt_Time_Accumulated + timer_count);
    timer_count_last = timer_count;
    /* enable interrupt */
    rt_hw_interrupt_enable(level);
    return Hrt_Time_now;
}

uint64_t hrt_absolute_ms(void)
{
    return hrt_absolute_time()/1000;
}


void hrt_delay_us(uint32_t us)
{
	uint64_t current_us;
	current_us = hrt_absolute_time();
	//轮训等待延迟时间到
	while(current_us+us >= hrt_absolute_time());
}

void hrt_delay_ms(uint32_t ms)
{
	uint64_t current_ms;
	current_ms = hrt_absolute_ms();
	//轮训等待延迟时间到
	while((current_ms+ms) >= hrt_absolute_ms());
}


