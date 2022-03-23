#include "driver_led.h"
#include "stm32f4xx_hal.h"
#include "rtthread.h"
#include "stdbool.h"
#include "driver_battery.h"

#define BAT_ADCx_CLK_ENABLE()				__HAL_RCC_ADC1_CLK_ENABLE()
#define BAT_ADCx_GPIO_CLK_ENABLE()      	__HAL_RCC_GPIOA_CLK_ENABLE()
#define BAT_ADCx_PIN                    	GPIO_PIN_4
#define BAT_ADCx_GPIO_PORT              	GPIOA
#define BAT_ADCx_DMA_STREAM					DMA2_Stream4
#define BAT_ADCx_DMA_CHANNEL				DMA_CHANNEL_0
#define BAT_ADCx_IRQn						ADC_IRQn
#define BAT_ADCx_DMA_IRQn					DMA2_Stream4_IRQn

uint16_t bat_adc_value = 0;
ADC_HandleTypeDef adc1_handler;//ADC句柄
static DMA_HandleTypeDef  hdma_adc;//dma数据接收

static void bat_adc_channel_config(void)   
{
    ADC_ChannelConfTypeDef adc1_channel_conf;
    adc1_channel_conf.Channel=4;                                   		//通道
    adc1_channel_conf.Rank=1;                                       	//1个序列
    adc1_channel_conf.SamplingTime=ADC_SAMPLETIME_480CYCLES;         	//采样时间
    adc1_channel_conf.Offset=0;                 
    HAL_ADC_ConfigChannel(&adc1_handler,&adc1_channel_conf);        	//通道配置
}

void bat_adc_init(void)
{ 
    adc1_handler.Instance=ADC1;
    adc1_handler.Init.ClockPrescaler=ADC_CLOCKPRESCALER_PCLK_DIV2; //2分频，ADCCLK=PCLK2/4=108/4=27MHZ
    adc1_handler.Init.Resolution=ADC_RESOLUTION_12B;             //12位模式
    adc1_handler.Init.DataAlign=ADC_DATAALIGN_RIGHT;             //右对齐
    adc1_handler.Init.ScanConvMode=DISABLE;                      //非扫描模式
    adc1_handler.Init.EOCSelection=DISABLE;                      //关闭EOC中断
    adc1_handler.Init.ContinuousConvMode=ENABLE;                //开启连续转换
    adc1_handler.Init.NbrOfConversion=1;                         //1个转换在规则序列中 也就是只转换规则序列1 
    adc1_handler.Init.DiscontinuousConvMode=DISABLE;             //禁止不连续采样模式
    adc1_handler.Init.NbrOfDiscConversion=0;                     //不连续采样通道数为0
    adc1_handler.Init.ExternalTrigConv=ADC_SOFTWARE_START;       //软件触发
    adc1_handler.Init.ExternalTrigConvEdge=ADC_EXTERNALTRIGCONVEDGE_NONE;//使用软件触发
    adc1_handler.Init.DMAContinuousRequests=DISABLE;             //关闭DMA请求
    HAL_ADC_Init(&adc1_handler);                                 //初始化 
	//通道ADC配置
	bat_adc_channel_config();
	HAL_ADC_Start(&adc1_handler);
}

uint16_t bat_get_adc_value(void)
{
	uint16_t adc_value = 0;
	if(HAL_ADC_PollForConversion(&adc1_handler,1) == HAL_OK)
	{
		adc_value = HAL_ADC_GetValue(&adc1_handler);
	}
	return adc_value;
}
void bat_adc_dma_init(void)
{
	__HAL_RCC_DMA2_CLK_ENABLE();
	hdma_adc.Instance = BAT_ADCx_DMA_STREAM;
	hdma_adc.Init.Channel  = BAT_ADCx_DMA_CHANNEL;
	hdma_adc.Init.Direction = DMA_PERIPH_TO_MEMORY;
	hdma_adc.Init.PeriphInc = DMA_PINC_DISABLE;
	hdma_adc.Init.MemInc = DMA_MINC_ENABLE;
	hdma_adc.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
	hdma_adc.Init.MemDataAlignment = DMA_PDATAALIGN_HALFWORD;
	hdma_adc.Init.Mode = DMA_CIRCULAR;
	hdma_adc.Init.Priority = DMA_PRIORITY_LOW;
	hdma_adc.Init.FIFOMode = DMA_FIFOMODE_DISABLE;         
	hdma_adc.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
	hdma_adc.Init.MemBurst = DMA_MBURST_SINGLE;
	hdma_adc.Init.PeriphBurst = DMA_PBURST_SINGLE; 

	HAL_DMA_Init(&hdma_adc);

	/* Associate the initialized DMA handle to the the ADC handle */
	__HAL_LINKDMA(&adc1_handler, DMA_Handle, hdma_adc);

	HAL_NVIC_SetPriority(BAT_ADCx_IRQn, 0, 0);   
	HAL_NVIC_EnableIRQ(BAT_ADCx_IRQn);
}
//ADC底层驱动，引脚配置，时钟使能
//此函数会被HAL_ADC_Init()调用
//hadc:ADC句柄
void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    BAT_ADCx_CLK_ENABLE();           
    BAT_ADCx_GPIO_CLK_ENABLE();					
	
    GPIO_InitStruct.Pin = BAT_ADCx_PIN;          //PA5
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;     //模拟
    GPIO_InitStruct.Pull = GPIO_NOPULL;          //不带上下拉
	GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(BAT_ADCx_GPIO_PORT,&GPIO_InitStruct);
}

void HAL_ADC_MspDeInit(ADC_HandleTypeDef *hadc)
{
	static DMA_HandleTypeDef  hdma_adc;
	__HAL_RCC_ADC_FORCE_RESET();
	__HAL_RCC_ADC_RELEASE_RESET();
	HAL_GPIO_DeInit(BAT_ADCx_GPIO_PORT, BAT_ADCx_PIN);
	if(hadc->DMA_Handle != RT_NULL)
	{
		HAL_DMA_DeInit(hadc->DMA_Handle); 
		HAL_NVIC_DisableIRQ(BAT_ADCx_DMA_IRQn);
	}
}
