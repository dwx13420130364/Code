#include "driver_led.h"
#include "stm32f4xx_hal.h"
#include "rtthread.h"
#include "stdbool.h"



#define INDICATOR_LED_GPIO_CLK_ENABLE()     	__HAL_RCC_GPIOC_CLK_ENABLE()
#define INDICATOR_LED_PIN                   	GPIO_PIN_1
#define INDICATOR_LED_GPIO_PORT             	GPIOC

#define Battery_LED_GPIO_CLK_ENABLE()     		__HAL_RCC_GPIOC_CLK_ENABLE()
#define Battery_LED_PIN                   		GPIO_PIN_0
#define Battery_LED_GPIO_PORT             		GPIOC

//indicator led
void battery_led_disable(void)
{
	HAL_GPIO_WritePin(Battery_LED_GPIO_PORT, Battery_LED_PIN, GPIO_PIN_RESET);
}

void battery_led_enable(void)
{
	HAL_GPIO_WritePin(Battery_LED_GPIO_PORT, Battery_LED_PIN, GPIO_PIN_SET);
}

void battery_led_init(void){
	GPIO_InitTypeDef  GPIO_InitStruct;
	
	Battery_LED_GPIO_CLK_ENABLE();
	GPIO_InitStruct.Pin       = Battery_LED_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(Battery_LED_GPIO_PORT, &GPIO_InitStruct);
	
	battery_led_disable();
}


//indicator led
void indicator_led_disable(void)
{
	HAL_GPIO_WritePin(INDICATOR_LED_GPIO_PORT, INDICATOR_LED_PIN, GPIO_PIN_RESET);
}

void indicator_led_enable(void)
{
	HAL_GPIO_WritePin(INDICATOR_LED_GPIO_PORT, INDICATOR_LED_PIN, GPIO_PIN_SET);
}

void indicator_led_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct;
	
	INDICATOR_LED_GPIO_CLK_ENABLE();
	GPIO_InitStruct.Pin       = INDICATOR_LED_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(INDICATOR_LED_GPIO_PORT, &GPIO_InitStruct);
	
	indicator_led_disable();
}
