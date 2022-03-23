#include "stm32f4xx.h"
#include "drv_usart.h"
#include "stm32f4xx_hal_uart.h"
#include "stm32f4xx_hal.h"
#include "stdlib.h"
#include "stdio.h"
#include "stdarg.h"

#define USART2_CLK_ENABLE()             __USART2_CLK_ENABLE()
#define USART2_RX_GPIO_CLK_ENABLE()    	__GPIOA_CLK_ENABLE()
#define USART2_TX_GPIO_CLK_ENABLE()    	__GPIOA_CLK_ENABLE()

/* Definition for USART2 Pins */
#define USART2_TX_PIN                   GPIO_PIN_2
#define USART2_TX_GPIO_PORT             GPIOA
#define USART2_TX_AF                    GPIO_AF7_USART2
#define USART2_RX_PIN                   GPIO_PIN_3
#define USART2_RX_GPIO_PORT             GPIOA
#define USART2_RX_AF                    GPIO_AF7_USART2

UART_HandleTypeDef console;

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef  GPIO_InitStruct;
	/* Enable GPIO TX/RX clock */
	USART2_TX_GPIO_CLK_ENABLE();
	USART2_RX_GPIO_CLK_ENABLE();

	/* UART TX GPIO pin configuration  */
	GPIO_InitStruct.Pin       = USART2_TX_PIN;
	GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull      = GPIO_PULLUP;
	GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
	GPIO_InitStruct.Alternate = USART2_TX_AF;
	HAL_GPIO_Init(USART2_TX_GPIO_PORT, &GPIO_InitStruct);

	/* UART RX GPIO pin configuration  */
	GPIO_InitStruct.Pin = USART2_RX_PIN;
	GPIO_InitStruct.Alternate = USART2_RX_AF;
	HAL_GPIO_Init(USART2_RX_GPIO_PORT, &GPIO_InitStruct);
}

void stm32_uart_init(void)
{
    USART2_CLK_ENABLE();
	console.Instance = USART2;
    console.Init.BaudRate   = 115200;
    console.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    console.Init.Mode       = UART_MODE_TX_RX;
	console.Init.WordLength = UART_WORDLENGTH_8B;
    console.Init.StopBits   = UART_STOPBITS_1;
	console.Init.Parity     = UART_PARITY_NONE;
    if (HAL_UART_Init(&console) != HAL_OK)
    {
       while(1){}
    }
    return;
}

static int stm32_putc(char c)
{
	//等待串口发送数据寄存器空
    while (!(console.Instance->SR & UART_FLAG_TXE));
	//将数据填充到数据寄存器
    console.Instance->DR = c;
    return 1;
}

static uint32_t stm32_putstr(char *data,uint32_t len)
{
	uint32_t send_len = 0;
	if(data == NULL)
		return 0;
	while(send_len <= len)
	{
		stm32_putc(*data);
		data++;
		send_len++;
	}
	return send_len;
}

#if 1
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 
}; 

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
void _sys_exit(int x) 
{ 
	x = x; 
} 

void _ttywrch(int ch)
{
	ch = ch;
}

//重定义fputc函数 
int fputc(int ch, FILE *f)
{ 	
	while ((__HAL_UART_GET_FLAG(&console, UART_FLAG_TXE) == RESET))
	{}
	console.Instance->DR = (uint8_t)ch;
	return ch;
}


#endif


static char rt_log_buf[1024];
void rt_kprintf(const char *fmt, ...)
{
    va_list args;
    uint16_t length;
 
    va_start(args, fmt);
    length = vsprintf(rt_log_buf, fmt, args);
    if (length > sizeof(rt_log_buf))
        length = sizeof(rt_log_buf);

	stm32_putstr(rt_log_buf,length);
    va_end(args);
}
