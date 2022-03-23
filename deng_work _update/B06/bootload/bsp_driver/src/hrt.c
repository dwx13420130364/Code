#include "rthw.h"
#include "hrt.h"
#include "stm32f7xx.h"
#include "finsh.h"
#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal_tim.h"

/* latency histogram */
#define LATENCY_BUCKET_COUNT 8
const uint16_t latency_bucket_count = LATENCY_BUCKET_COUNT;
const uint16_t	latency_buckets[LATENCY_BUCKET_COUNT] = { 1, 2, 5, 10, 20, 50, 100, 1000 };
uint32_t	latency_counters[LATENCY_BUCKET_COUNT + 1];

#define HRT_COUNTER_SCALE(_c)	(_c)
#define PERIOD_HRT_TIMER    1000//50000//

TIM_HandleTypeDef HRT_TIMER_Handler;      //定时器句柄 
void hrt_init(void)
{
	HRT_TIMER_Handler.Instance=HRT_TIMER;
    HRT_TIMER_Handler.Init.Prescaler=215;
    HRT_TIMER_Handler.Init.CounterMode=TIM_COUNTERMODE_UP;
    HRT_TIMER_Handler.Init.Period=PERIOD_HRT_TIMER - 1;
    HRT_TIMER_Handler.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_Base_Init(&HRT_TIMER_Handler);
    
    HAL_TIM_Base_Start_IT(&HRT_TIMER_Handler);
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    if(htim->Instance==TIM4)
	{
		__HAL_RCC_TIM4_CLK_ENABLE();            //使能TIM4时钟
		HAL_NVIC_SetPriority(TIM4_IRQn,0,0);    //设置中断优先级，抢占优先级0，子优先级0
		HAL_NVIC_EnableIRQ(TIM4_IRQn);          //开启ITM4中断   
	}  
	
	if(htim->Instance==TIM5)
	{
		__HAL_RCC_TIM5_CLK_ENABLE();            //使能TIM5时钟
		HAL_NVIC_SetPriority(TIM5_IRQn,0,0);    //设置中断优先级，抢占优先级0，子优先级0
		HAL_NVIC_EnableIRQ(TIM5_IRQn);          //开启ITM5中断   
	}  
}

static volatile hrt_abstime Hrt_Time_Accumulated = 0;
static volatile uint32_t timer_count_last = 0;
hrt_abstime hrt_absolute_time(void)
{
    register rt_base_t level;
    uint32_t timer_count = 0;
    hrt_abstime Hrt_Time_now = 0;

    /* disable interrupt */
    level = rt_hw_interrupt_disable();
    timer_count = HRT_TIMER->CNT;
    if((timer_count < timer_count_last)&&(timer_count_last != (PERIOD_HRT_TIMER - 1)))
    {
        Hrt_Time_Accumulated += PERIOD_HRT_TIMER;
    }
    Hrt_Time_now = HRT_COUNTER_SCALE(Hrt_Time_Accumulated + timer_count);
    timer_count_last = timer_count;
    /* enable interrupt */
    rt_hw_interrupt_enable(level);

    return Hrt_Time_now;
}

hrt_abstime hrt_absolute_ms(void)
{
    register rt_base_t level;
    static uint32_t timer_count = 0;
    static hrt_abstime Hrt_Time_now = 0;

    /* disable interrupt */
    level = rt_hw_interrupt_disable();
    timer_count = HRT_TIMER->CNT;
    if((timer_count < timer_count_last)&&(timer_count_last != (PERIOD_HRT_TIMER - 1)))
    {
        Hrt_Time_Accumulated += PERIOD_HRT_TIMER;
    }
    Hrt_Time_now = HRT_COUNTER_SCALE(Hrt_Time_Accumulated + timer_count)/1000;
    timer_count_last = timer_count;
    /* enable interrupt */
    rt_hw_interrupt_enable(level);

    return Hrt_Time_now;
}

static hrt_abstime utc_time_offset_us = 0;
hrt_abstime hrt_utc_us(void)
{
    hrt_abstime Hrt_Time_now = 0;

	Hrt_Time_now = hrt_absolute_time() + utc_time_offset_us;

    return Hrt_Time_now;
}

int64_t delta_utc_us = 0;
void set_utctime_offset(hrt_abstime current_utc_us)
{
    static hrt_abstime last_t = 0;
    
	utc_time_offset_us = current_utc_us - hrt_absolute_time();
    delta_utc_us = utc_time_offset_us - last_t;
//    rt_kprintf("[SYSTIME]utc offset delta us: %lld\n", utc_time_offset_us - last_t);
    last_t = utc_time_offset_us;
}

void utcoffset(void)
{
    uint32_t offset;
    offset = utc_time_offset_us * 0.001f;
    rt_kprintf("[SYSTIME]utc offset: %d\n", offset);
}

hrt_abstime hrt_elapsed_time(hrt_abstime time)
{
    register rt_base_t level;
    hrt_abstime Hrt_Time_now = 0;
    hrt_abstime Hrt_Time_diff = 0;
    uint32_t timer_count = 0;

    /* disable interrupt */
    level = rt_hw_interrupt_disable();
    timer_count = HRT_TIMER->CNT;
    if((timer_count < timer_count_last)&&(timer_count_last != (PERIOD_HRT_TIMER - 1)))
    {
        Hrt_Time_Accumulated += PERIOD_HRT_TIMER;
    }
    Hrt_Time_now = HRT_COUNTER_SCALE(Hrt_Time_Accumulated + timer_count);
    timer_count_last = timer_count;
    if(Hrt_Time_now > time)
    {
        Hrt_Time_diff = Hrt_Time_now - time;
    }
    else
    {
        Hrt_Time_diff = 0;
    }
    /* enable interrupt */
    rt_hw_interrupt_enable(level);
    
    return Hrt_Time_diff;
}

hrt_abstime hrt_elapsed_time1(const volatile hrt_abstime *time)
{
    register rt_base_t level;
    hrt_abstime Hrt_Time_now = 0;
    hrt_abstime Hrt_Time_diff = 0;
    uint32_t timer_count = 0;

    /* disable interrupt */
    level = rt_hw_interrupt_disable();
    timer_count = HRT_TIMER->CNT;
    if((timer_count < timer_count_last)&&(timer_count_last != (PERIOD_HRT_TIMER - 1)))
    {
        Hrt_Time_Accumulated += PERIOD_HRT_TIMER;
    }
    Hrt_Time_now = HRT_COUNTER_SCALE(Hrt_Time_Accumulated + timer_count);
    timer_count_last = timer_count;
    if(Hrt_Time_now > *time)
    {
        Hrt_Time_diff = Hrt_Time_now - *time;
    }
    else
    {
        Hrt_Time_diff = 0;
    }
    /* enable interrupt */
    rt_hw_interrupt_enable(level);
    
    return Hrt_Time_diff;
}

hrt_abstime hrt_elapsed_ms(hrt_abstime time)
{
    register rt_base_t level;
    hrt_abstime Hrt_Time_now = 0;
    hrt_abstime Hrt_Time_diff = 0;
    uint32_t timer_count = 0;

    /* disable interrupt */
    level = rt_hw_interrupt_disable();
    timer_count = HRT_TIMER->CNT;
    if((timer_count < timer_count_last)&&(timer_count_last != (PERIOD_HRT_TIMER - 1)))
    {
        Hrt_Time_Accumulated += PERIOD_HRT_TIMER;
    }
    Hrt_Time_now = HRT_COUNTER_SCALE(Hrt_Time_Accumulated + timer_count);
    timer_count_last = timer_count;
    if(Hrt_Time_now>time)
    {
        Hrt_Time_diff = (Hrt_Time_now - time)/1000;
    }
    else
    {
        Hrt_Time_diff = 0;
    }
    /* enable interrupt */
    rt_hw_interrupt_enable(level);
    
    return Hrt_Time_diff;
}

void hrt_delay_us(uint32_t us)
{
	hrt_abstime current_us;
	current_us = hrt_absolute_time();
	
	while(current_us+us >= hrt_absolute_time());
}

void hrt_delay_ms(uint32_t ms)
{
	hrt_abstime current_ms;
	current_ms = hrt_absolute_ms();
	
	while((current_ms+ms) >= hrt_absolute_ms());
}

#if   USE_HRT_TIMER == 4

void TIM4_IRQHandler(void)	
{
    /* enter interrupt */
    rt_interrupt_enter();
	if(__HAL_TIM_GET_FLAG(&HRT_TIMER_Handler, TIM_FLAG_UPDATE) != RESET)
	{
		if(__HAL_TIM_GET_IT_SOURCE(&HRT_TIMER_Handler, TIM_IT_UPDATE) !=RESET)
		{
			Hrt_Time_Accumulated = HRT_COUNTER_SCALE(Hrt_Time_Accumulated + PERIOD_HRT_TIMER);
			HAL_TIM_PeriodElapsedCallback(&HRT_TIMER_Handler);
			HAL_TIM_PeriodElapsedCallback(&HRT_TIMER_Handler);
			__HAL_TIM_CLEAR_IT(&HRT_TIMER_Handler, TIM_IT_UPDATE);
		}
	}
    /* leave interrupt */
    rt_interrupt_leave();
}

#endif
//uint32_t time_cnt = 0;
//uint32_t time_cnt_convert = 0;
#if   USE_HRT_TIMER == 5
void TIM5_IRQHandler(void)	
{
    /* enter interrupt */
    rt_interrupt_enter();
	if(__HAL_TIM_GET_FLAG(&HRT_TIMER_Handler, TIM_FLAG_UPDATE) != RESET)
	{
		if(__HAL_TIM_GET_IT_SOURCE(&HRT_TIMER_Handler, TIM_IT_UPDATE) !=RESET)
		{
			timer_count_last = HRT_TIMER->CNT;
			Hrt_Time_Accumulated = HRT_COUNTER_SCALE(Hrt_Time_Accumulated + PERIOD_HRT_TIMER);
//			HAL_TIM_PeriodElapsedCallback(&HRT_TIMER_Handler);
			__HAL_TIM_CLEAR_IT(&HRT_TIMER_Handler, TIM_IT_UPDATE);
//			time_cnt++;
//			time_cnt_convert = time_cnt * 50;
//			rt_kprintf("a");
//			HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_10);
		}
	}
    /* leave interrupt */
    rt_interrupt_leave();
}

#endif

FINSH_FUNCTION_EXPORT(utcoffset, get utc offset.)