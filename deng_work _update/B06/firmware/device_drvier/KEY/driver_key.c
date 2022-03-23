#include "driver_key.h"
#include "stm32f4xx_hal.h"
#include "rtthread.h"
#include "stdbool.h"


#define UWB_POWER_GPIO_CLK_ENABLE()     		__HAL_RCC_GPIOA_CLK_ENABLE()
#define UWB_POWER_PIN                    		GPIO_PIN_5
#define UWB_POWER_GPIO_PORT              		GPIOA

#define WIFI_POWER_GPIO_CLK_ENABLE()      		__HAL_RCC_GPIOC_CLK_ENABLE()
#define WIFI_POWER_PIN                    		GPIO_PIN_3
#define WIFI_POWER_GPIO_PORT              		GPIOC






void uwb_power_out_enable(void)
{
	HAL_GPIO_WritePin(UWB_POWER_GPIO_PORT, UWB_POWER_PIN, GPIO_PIN_SET);
}

void uwb_power_out_disable(void)
{
	HAL_GPIO_WritePin(UWB_POWER_GPIO_PORT, UWB_POWER_PIN, GPIO_PIN_RESET);
}

void uwb_power_out_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct;
	//使能时钟
	UWB_POWER_GPIO_CLK_ENABLE();

	//GPIO初始化
	GPIO_InitStruct.Pin       = UWB_POWER_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(UWB_POWER_GPIO_PORT, &GPIO_InitStruct);
	
	//输出低电平
	uwb_power_out_disable();
}




void wifi_power_out_enable(void)
{
	HAL_GPIO_WritePin(WIFI_POWER_GPIO_PORT, WIFI_POWER_PIN, GPIO_PIN_SET);
}

void wifi_power_out_disable(void)
{
	HAL_GPIO_WritePin(WIFI_POWER_GPIO_PORT, WIFI_POWER_PIN, GPIO_PIN_RESET);
}

void wifi_power_out_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct;
	//使能时钟
	WIFI_POWER_GPIO_CLK_ENABLE();

	//GPIO初始化
	GPIO_InitStruct.Pin       = WIFI_POWER_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(WIFI_POWER_GPIO_PORT, &GPIO_InitStruct);
	
	//输出低电平
	wifi_power_out_disable();
}



