#include "bsp_led.h"
#include "stm32f4xx_hal.h"
#include "stdbool.h"

#define POWER_LED_GPIO_CLK_ENABLE()      	__HAL_RCC_GPIOC_CLK_ENABLE()
#define POWER_LED_PIN                    	GPIO_PIN_1
#define POWER_LED_GPIO_PORT              	GPIOC

#define INDICATOR_LED_GPIO_CLK_ENABLE()     __HAL_RCC_GPIOC_CLK_ENABLE()
#define INDICATOR_LED_PIN                   GPIO_PIN_0
#define INDICATOR_LED_GPIO_PORT             GPIOC

#define STATUS_LED_GPIO_CLK_ENABLE()      	__HAL_RCC_GPIOE_CLK_ENABLE()
#define STATUS_LED_PIN                    	GPIO_PIN_11
#define STATUS_LED_GPIO_PORT              	GPIOE

//power led
void power_led_enable(void)
{
	HAL_GPIO_WritePin(POWER_LED_GPIO_PORT, POWER_LED_PIN, GPIO_PIN_SET);
}

void power_led_disable(void)
{
	HAL_GPIO_WritePin(POWER_LED_GPIO_PORT, POWER_LED_PIN, GPIO_PIN_RESET);
}

void power_led_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct;
	//使能时钟
	POWER_LED_GPIO_CLK_ENABLE();

	//GPIO初始化
	GPIO_InitStruct.Pin       = POWER_LED_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(POWER_LED_GPIO_PORT, &GPIO_InitStruct);
	
	power_led_disable();
}

//indicator led
void indicator_led_enable(void)
{
	HAL_GPIO_WritePin(INDICATOR_LED_GPIO_PORT, INDICATOR_LED_PIN, GPIO_PIN_SET);
}

void indicator_led_disable(void)
{
	HAL_GPIO_WritePin(INDICATOR_LED_GPIO_PORT, INDICATOR_LED_PIN, GPIO_PIN_RESET);
}

void indicator_led_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct;
	//使能时钟
	POWER_LED_GPIO_CLK_ENABLE();

	//GPIO初始化
	GPIO_InitStruct.Pin       = INDICATOR_LED_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(INDICATOR_LED_GPIO_PORT, &GPIO_InitStruct);
	
	indicator_led_disable();
}

//stauts led
void status_led_enable(void)
{
	HAL_GPIO_WritePin(STATUS_LED_GPIO_PORT, STATUS_LED_PIN, GPIO_PIN_SET);
}

void status_led_disable(void)
{
	HAL_GPIO_WritePin(STATUS_LED_GPIO_PORT, STATUS_LED_PIN, GPIO_PIN_RESET);
}

void status_led_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct;
	//使能时钟
	POWER_LED_GPIO_CLK_ENABLE();

	//GPIO初始化
	GPIO_InitStruct.Pin       = STATUS_LED_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(STATUS_LED_GPIO_PORT, &GPIO_InitStruct);
	
	status_led_disable();
}
