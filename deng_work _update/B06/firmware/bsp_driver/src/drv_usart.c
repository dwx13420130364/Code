#include "stm32f4xx.h"
#include "drv_usart.h"
#include "board.h"
#include "stm32f4xx_hal_uart.h"
#include "stm32f4xx_hal.h"

#include <rtdevice.h>

#ifdef RT_USING_UART1
/* Definition for USART1 clock resources */
#define USART1_CLK_ENABLE()              __USART1_CLK_ENABLE()
#define USART1_RX_GPIO_CLK_ENABLE()      __GPIOA_CLK_ENABLE()
#define USART1_TX_GPIO_CLK_ENABLE()      __GPIOA_CLK_ENABLE()

#define USART1_FORCE_RESET()            __USART1_FORCE_RESET()
#define USART1_RELEASE_RESET()          __USART1_RELEASE_RESET()

/* Definition for USART1 Pins */
#define USART1_TX_PIN                   GPIO_PIN_9
#define USART1_TX_GPIO_PORT             GPIOA
#define USART1_TX_AF                    GPIO_AF7_USART1
#define USART1_RX_PIN                   GPIO_PIN_10
#define USART1_RX_GPIO_PORT             GPIOA
#define USART1_RX_AF                    GPIO_AF7_USART1

#define UART1_TX_DMA        			DMA2_Stream7
#define UART1_RX_DMA        			DMA2_Stream2
#define UART1_TX_DMA_IRQ    			DMA2_Stream7_IRQn
#define UART1_RX_DMA_IRQ    			DMA2_Stream2_IRQn
#endif

#ifdef RT_USING_UART2
/* Definition for USART2 clock resources */
#define USART2_CLK_ENABLE()             __USART2_CLK_ENABLE()
#define USART2_RX_GPIO_CLK_ENABLE()    	__GPIOA_CLK_ENABLE()
#define USART2_TX_GPIO_CLK_ENABLE()    	__GPIOA_CLK_ENABLE()

#define USART2_FORCE_RESET()           	__USART2_FORCE_RESET()
#define USART2_RELEASE_RESET()         	__USART2_RELEASE_RESET()

/* Definition for USART2 Pins */
#define USART2_TX_PIN                   GPIO_PIN_2
#define USART2_TX_GPIO_PORT             GPIOA
#define USART2_TX_AF                    GPIO_AF7_USART2
#define USART2_RX_PIN                   GPIO_PIN_3
#define USART2_RX_GPIO_PORT             GPIOA
#define USART2_RX_AF                    GPIO_AF7_USART2

#define UART2_TX_DMA        			DMA1_Stream6
#define UART2_RX_DMA        			DMA1_Stream5
#define UART2_TX_DMA_IRQ    			DMA1_Stream6_IRQn
#define UART2_RX_DMA_IRQ    			DMA1_Stream5_IRQn
#endif

#ifdef RT_USING_UART6
/* Definition for USART3 clock resources */
#define USART6_CLK_ENABLE()             __USART6_CLK_ENABLE()
#define USART6_RX_GPIO_CLK_ENABLE()     __GPIOC_CLK_ENABLE()
#define USART6_TX_GPIO_CLK_ENABLE()     __GPIOC_CLK_ENABLE()

#define USART6_FORCE_RESET()            __USART6_FORCE_RESET()
#define USART6_RELEASE_RESET()          __USART6_RELEASE_RESET()

/* Definition for USART3 Pins */
#define USART6_TX_PIN                   GPIO_PIN_6
#define USART6_TX_GPIO_PORT             GPIOC
#define USART6_TX_AF                    GPIO_AF8_USART6
#define USART6_RX_PIN                   GPIO_PIN_7
#define USART6_RX_GPIO_PORT             GPIOC
#define USART6_RX_AF                    GPIO_AF8_USART6

#define UART6_TX_DMA        			DMA2_Stream6
#define UART6_RX_DMA        			DMA2_Stream1
#define UART6_TX_DMA_IRQ    			DMA2_Stream6_IRQn
#define UART6_RX_DMA_IRQ    			DMA2_Stream1_IRQn
#endif

#ifdef RT_USING_UART1
struct rt_serial_device serial1;
static uint8_t uart1_dma_rx_buf_0[1040] = {0};
static uint8_t uart1_dma_rx_buf_1[1040] = {0};
static uint8_t uart1_rx_choise = 0;
#endif

#ifdef RT_USING_UART2
struct rt_serial_device serial2;
static uint8_t uart2_dma_rx_buf_0[256] = {0};
static uint8_t uart2_dma_rx_buf_1[256] = {0};
static uint8_t uart2_rx_choise = 0;

#endif

#ifdef RT_USING_UART6
struct rt_serial_device serial6;
static uint8_t uart6_dma_rx_buf_0[1040] = {0};
static uint8_t uart6_dma_rx_buf_1[1040] = {0};
static uint8_t uart6_rx_choise = 0;

#endif

uint32_t dma_recv_len = 0;
static rt_err_t stm32_configure(struct rt_serial_device *serial, struct serial_configure *cfg)
{
    struct stm32_uart *uart;

    RT_ASSERT(serial != RT_NULL);
    RT_ASSERT(cfg != RT_NULL);

    uart = (struct stm32_uart *)serial->parent.user_data;

    uart->UartHandle.Init.BaudRate   = cfg->baud_rate;
    uart->UartHandle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    uart->UartHandle.Init.Mode       = UART_MODE_TX_RX;

    switch (cfg->data_bits)
    {
    case DATA_BITS_8:
        uart->UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
        break;
    case DATA_BITS_9:
        uart->UartHandle.Init.WordLength = UART_WORDLENGTH_9B;
        break;
    default:
        uart->UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
        break;
    }
    switch (cfg->stop_bits)
    {
    case STOP_BITS_1:
        uart->UartHandle.Init.StopBits   = UART_STOPBITS_1;
        break;
    case STOP_BITS_2:
        uart->UartHandle.Init.StopBits   = UART_STOPBITS_2;
        break;
    default:
        uart->UartHandle.Init.StopBits   = UART_STOPBITS_1;
        break;
    }
    switch (cfg->parity)
    {
    case PARITY_NONE:
        uart->UartHandle.Init.Parity     = UART_PARITY_NONE;
        break;
    case PARITY_ODD:
        uart->UartHandle.Init.Parity     = UART_PARITY_ODD;
        break;
    case PARITY_EVEN:
        uart->UartHandle.Init.Parity     = UART_PARITY_EVEN;
        break;
    default:
        uart->UartHandle.Init.Parity     = UART_PARITY_NONE;
        break;
    }
	
    if (HAL_UART_Init(&uart->UartHandle) != HAL_OK)
    {
        return RT_ERROR;
    }
    return RT_EOK;
}

static rt_err_t stm32_control(struct rt_serial_device *serial, int cmd, void *arg)
{
    struct stm32_uart *uart;

    RT_ASSERT(serial != RT_NULL);
    uart = (struct stm32_uart *)serial->parent.user_data;
    switch (cmd)
    {
    case RT_DEVICE_CTRL_CLR_INT:
		if(serial->parent.flag & RT_DEVICE_FLAG_INT_RX)
		{
			/* disable rx irq */
			UART_DISABLE_IRQ(uart->irq);
			/* disable interrupt */
			__HAL_UART_DISABLE_IT(&uart->UartHandle, UART_IT_RXNE);
		}
		if(serial->parent.flag & RT_DEVICE_FLAG_DMA_RX)
		{
			/* disable rx irq */
			UART_DISABLE_IRQ(uart->irq);
			/* disable interrupt */
			HAL_NVIC_DisableIRQ(uart->dma_rx_irq);
			__HAL_UART_DISABLE_IT(&uart->UartHandle,UART_IT_IDLE);
		}
		if(serial->parent.flag & RT_DEVICE_FLAG_DMA_TX)
		{
			/* disable rx irq */
			UART_DISABLE_IRQ(uart->irq);
			/* disable interrupt */
			HAL_NVIC_DisableIRQ(uart->dma_tx_irq);
		}
        break;
    case RT_DEVICE_CTRL_SET_INT:
		if(serial->parent.flag & RT_DEVICE_FLAG_INT_RX)
		{
			/* enable rx irq */
			UART_ENABLE_IRQ(uart->irq);
			/* enable interrupt */
			__HAL_UART_ENABLE_IT(&uart->UartHandle, UART_IT_RXNE);
		}
		if(serial->parent.flag & RT_DEVICE_FLAG_DMA_RX)
		{
			/* enable rx irq */
			UART_ENABLE_IRQ(uart->irq);
			HAL_NVIC_EnableIRQ(uart->dma_rx_irq);
			__HAL_UART_ENABLE_IT(&uart->UartHandle,UART_IT_IDLE);
			if(serial == &serial1)
			{
				uart1_rx_choise = 0;
				HAL_UART_Receive_DMA(&uart->UartHandle,uart1_dma_rx_buf_0,sizeof(uart1_dma_rx_buf_0));
			}
			if(serial == &serial2)
			{
				uart2_rx_choise = 0;
				HAL_UART_Receive_DMA(&uart->UartHandle,uart2_dma_rx_buf_0,sizeof(uart2_dma_rx_buf_0));
			}
			if(serial == &serial6)
			{
				uart6_rx_choise = 0;
				HAL_UART_Receive_DMA(&uart->UartHandle,uart6_dma_rx_buf_0,sizeof(uart6_dma_rx_buf_0));
			}
		}
		if(serial->parent.flag & RT_DEVICE_FLAG_DMA_TX)
		{
			/* enable rx irq */
			UART_ENABLE_IRQ(uart->irq);
			HAL_NVIC_EnableIRQ(uart->dma_tx_irq);
		}
        break;
    }

    return RT_EOK;
}

static int stm32_putc(struct rt_serial_device *serial, char c)
{
    struct stm32_uart *uart;

    RT_ASSERT(serial != RT_NULL);
    uart = (struct stm32_uart *)serial->parent.user_data;

    while (!(uart->UartHandle.Instance->SR & UART_FLAG_TXE));
    uart->UartHandle.Instance->DR = c;

    return 1;
}

static int stm32_getc(struct rt_serial_device *serial)
{
    int ch;
    struct stm32_uart *uart;

    RT_ASSERT(serial != RT_NULL);
    uart = (struct stm32_uart *)serial->parent.user_data;

    ch = -1;
    if (uart->UartHandle.Instance->SR & UART_FLAG_RXNE)
    {
        ch = uart->UartHandle.Instance->DR & 0xff;
    }

    return ch;
}

static void rt_serial_enable_dma(struct stm32_uart *uart, const rt_uint8_t *buf, rt_size_t size)
{
	HAL_UART_Transmit_DMA(&uart->UartHandle, (rt_uint8_t *)buf, size);
}

rt_size_t stm32_dma_transmit(struct rt_serial_device *serial, const rt_uint8_t *buf, rt_size_t size, int direction)
{
	rt_size_t ret = 0;
    struct stm32_uart *uart;
    
    RT_ASSERT(serial != RT_NULL);
    uart = (struct stm32_uart *)serial->parent.user_data;
	if (direction == RT_SERIAL_DMA_TX)
    {
        /*DMA tx mode*/
        rt_serial_enable_dma(uart, buf, size);
        return size;
    }
    else if (direction == RT_SERIAL_DMA_RX)
    {
        return rt_ringbuffer_get(&((struct rt_serial_rx_dma*) (serial->serial_rx))->ringbuffer ,(rt_uint8_t *)buf, size);
    }
    return ret;
}

static const struct rt_uart_ops stm32_uart_ops =
{
    stm32_configure,
    stm32_control,
    stm32_putc,
    stm32_getc,
    stm32_dma_transmit
};

#ifdef RT_USING_UART1
/* UART1 device driver structure */
struct stm32_uart uart1 =
{
    USART1,
    USART1_IRQn,
    0,
    0,
    UART1_TX_DMA,
    UART1_RX_DMA,
    UART1_TX_DMA_IRQ,
    UART1_RX_DMA_IRQ,
	DMA_CHANNEL_4,
	DMA_CHANNEL_4,
	0,
	0
};

void USART1_IRQHandler(void)
{
    struct stm32_uart *uart;
    rt_uint32_t temp = 0;
	rt_uint32_t len = 0;
    uart = &uart1;
   
    /* enter interrupt */
    rt_interrupt_enter();

	if(serial1.parent.open_flag&RT_DEVICE_FLAG_DMA_RX)
	{
		if (__HAL_UART_GET_FLAG(&uart->UartHandle,UART_FLAG_IDLE) != RESET)
		{
			//清除uart总线空闲中断
			__HAL_UART_CLEAR_IDLEFLAG(&uart->UartHandle);
			temp = uart->UartHandle.Instance->SR;
			temp = uart->UartHandle.Instance->DR;
			//关闭串口DMA传输
			HAL_UART_RX_DMAStop(&uart->UartHandle);
			// 获取DMA中未传输的数据个数，NDTR寄存器分析见下面
			if(uart1_rx_choise)
			{
				uart1_rx_choise = 0;
				temp = uart->UARTRxDMA_Handler.Instance->NDTR;
				len = sizeof(uart1_dma_rx_buf_1) - temp;
				//切换到另外数组数据接收
				HAL_UART_Receive_DMA(&uart->UartHandle,uart1_dma_rx_buf_0,sizeof(uart1_dma_rx_buf_0));
				//将数据推入缓冲区
				rt_ringbuffer_put(&((struct rt_serial_rx_dma*) (serial1.serial_rx))->ringbuffer, uart1_dma_rx_buf_1, len);
				rt_hw_serial_isr(&serial1, RT_SERIAL_EVENT_RX_DMADONE);
			}
			else
			{
				uart1_rx_choise = 1;
				temp = uart->UARTRxDMA_Handler.Instance->NDTR;
				len = sizeof(uart1_dma_rx_buf_0) - temp;
				//切换到另外数组数据接收
				HAL_UART_Receive_DMA(&uart->UartHandle,uart1_dma_rx_buf_1,sizeof(uart1_dma_rx_buf_1));
				//将数据推入缓冲区
				rt_ringbuffer_put(&((struct rt_serial_rx_dma*) (serial1.serial_rx))->ringbuffer, uart1_dma_rx_buf_0, len);
				rt_hw_serial_isr(&serial1, RT_SERIAL_EVENT_RX_DMADONE);
			}
			dma_recv_len += len;
		}	
	}
	else
	{
		/* UART in mode Receiver ---------------------------------------------------*/
		if ((__HAL_USART_GET_FLAG(&uart->UartHandle, USART_FLAG_RXNE) != RESET) && (__HAL_UART_GET_IT_SOURCE(&uart->UartHandle, UART_IT_RXNE) != RESET))
		{
			rt_hw_serial_isr(&serial1, RT_SERIAL_EVENT_RX_IND);
			/* Clear RXNE interrupt flag */
			__HAL_USART_CLEAR_FLAG(&uart->UartHandle, USART_FLAG_RXNE);
		}
		/* UART in mode idle  ---------------------------------------------------*/
		if (__HAL_UART_GET_FLAG(&uart->UartHandle,UART_FLAG_IDLE) != RESET)
		{
			/* Clear idle interrupt flag */
			__HAL_UART_CLEAR_IDLEFLAG(&uart->UartHandle);
		}
	}
	/* UART in send mode  ------------------------------------------------------*/
	if(__HAL_USART_GET_FLAG(&uart->UartHandle,USART_FLAG_TXE) != RESET)
	{
//		 uart->UartHandle.Instance->DR = TxBuffer[]
	}
	/* UART parity error interrupt occurred -------------------------------------*/
    if ((__HAL_USART_GET_FLAG(&uart->UartHandle, USART_FLAG_PE) != RESET)) {
        __HAL_USART_CLEAR_FLAG(&uart->UartHandle, USART_FLAG_PE);
    }

    /* UART frame error interrupt occurred --------------------------------------*/
    if ((__HAL_USART_GET_FLAG(&uart->UartHandle, USART_FLAG_FE) != RESET)) {
        __HAL_USART_CLEAR_FLAG(&uart->UartHandle, USART_FLAG_FE);
    }

    /* UART noise error interrupt occurred --------------------------------------*/
    if ((__HAL_USART_GET_FLAG(&uart->UartHandle, USART_FLAG_NE) != RESET)) {
        __HAL_USART_CLEAR_FLAG(&uart->UartHandle, USART_FLAG_NE);
    }

    /* UART Over-Run interrupt occurred -----------------------------------------*/
    if ((__HAL_USART_GET_FLAG(&uart->UartHandle, USART_FLAG_ORE) != RESET)) {
        __HAL_USART_CLEAR_FLAG(&uart->UartHandle, USART_FLAG_ORE);
    }
	
	HAL_UART_IRQHandler(&uart->UartHandle);
    /* leave interrupt */
    rt_interrupt_leave();
}

void DMA2_Stream7_IRQHandler(void)
{
	HAL_DMA_IRQHandler(&uart1.UARTTxDMA_Handler);
}
void DMA2_Stream2_IRQHandler(void)
{
	HAL_DMA_IRQHandler(&uart1.UARTRxDMA_Handler);
}
#endif /* RT_USING_UART1 */

#ifdef RT_USING_UART2
/* UART2 device driver structure */
struct stm32_uart uart2 =
{
    USART2,
    USART2_IRQn,
    0,
    0,
    UART2_TX_DMA,
    UART2_RX_DMA,
    UART2_TX_DMA_IRQ,
    UART2_RX_DMA_IRQ,
	DMA_CHANNEL_4,
	DMA_CHANNEL_4,
	0,
	0
};

void USART2_IRQHandler(void)
{
    struct stm32_uart *uart;
	rt_uint32_t temp = 0;
	rt_uint32_t len = 0;
    uart = &uart2;

    /* enter interrupt */
    rt_interrupt_enter();

	if(serial2.parent.open_flag&RT_DEVICE_FLAG_DMA_RX)
	{
		if (__HAL_UART_GET_FLAG(&uart->UartHandle,UART_FLAG_IDLE) != RESET)
		{
			//清除uart总线空闲中断
			__HAL_UART_CLEAR_IDLEFLAG(&uart->UartHandle);
			temp = uart->UartHandle.Instance->SR;
			temp = uart->UartHandle.Instance->DR;
			//关闭串口DMA传输
			HAL_UART_RX_DMAStop(&uart->UartHandle);
			// 获取DMA中未传输的数据个数，NDTR寄存器分析见下面
			if(uart2_rx_choise)
			{
				uart2_rx_choise = 0;
				temp = uart->UARTRxDMA_Handler.Instance->NDTR;
				len = sizeof(uart2_dma_rx_buf_1) - temp;
				//切换到另外数组数据接收
				HAL_UART_Receive_DMA(&uart->UartHandle,uart2_dma_rx_buf_0,sizeof(uart2_dma_rx_buf_0));
				//将数据推入缓冲区
				rt_ringbuffer_put(&((struct rt_serial_rx_dma*) (serial2.serial_rx))->ringbuffer, uart2_dma_rx_buf_1, len);
				rt_hw_serial_isr(&serial2, RT_SERIAL_EVENT_RX_DMADONE);
			}
			else
			{
				uart2_rx_choise = 1;
				temp = uart->UARTRxDMA_Handler.Instance->NDTR;
				len = sizeof(uart2_dma_rx_buf_0) - temp;
				//切换到另外数组数据接收
				HAL_UART_Receive_DMA(&uart->UartHandle,uart2_dma_rx_buf_1,sizeof(uart2_dma_rx_buf_1));
				//将数据推入缓冲区
				rt_ringbuffer_put(&((struct rt_serial_rx_dma*) (serial2.serial_rx))->ringbuffer, uart2_dma_rx_buf_0, len);
				rt_hw_serial_isr(&serial2, RT_SERIAL_EVENT_RX_DMADONE);
			}
		}	
	}
	else
	{
		/* UART in mode Receiver ---------------------------------------------------*/
		if ((__HAL_USART_GET_FLAG(&uart->UartHandle, USART_FLAG_RXNE) != RESET) && (__HAL_UART_GET_IT_SOURCE(&uart->UartHandle, UART_IT_RXNE) != RESET))
		{
			rt_hw_serial_isr(&serial2, RT_SERIAL_EVENT_RX_IND);
			/* Clear RXNE interrupt flag */
			__HAL_USART_CLEAR_FLAG(&uart->UartHandle, USART_FLAG_RXNE);
		}
		/* UART in mode idle  ---------------------------------------------------*/
		if (__HAL_UART_GET_FLAG(&uart->UartHandle,UART_FLAG_IDLE) != RESET)
		{
			/* Clear idle interrupt flag */
			__HAL_UART_CLEAR_IDLEFLAG(&uart->UartHandle);
		}
	}
	
	/* UART parity error interrupt occurred -------------------------------------*/
    if ((__HAL_USART_GET_FLAG(&uart->UartHandle, USART_FLAG_PE) != RESET)) {
        __HAL_USART_CLEAR_FLAG(&uart->UartHandle, USART_FLAG_PE);
    }

    /* UART frame error interrupt occurred --------------------------------------*/
    if ((__HAL_USART_GET_FLAG(&uart->UartHandle, USART_FLAG_FE) != RESET)) {
        __HAL_USART_CLEAR_FLAG(&uart->UartHandle, USART_FLAG_FE);
    }

    /* UART noise error interrupt occurred --------------------------------------*/
    if ((__HAL_USART_GET_FLAG(&uart->UartHandle, USART_FLAG_NE) != RESET)) {
        __HAL_USART_CLEAR_FLAG(&uart->UartHandle, USART_FLAG_NE);
    }

    /* UART Over-Run interrupt occurred -----------------------------------------*/
    if ((__HAL_USART_GET_FLAG(&uart->UartHandle, USART_FLAG_ORE) != RESET)) {
        __HAL_USART_CLEAR_FLAG(&uart->UartHandle, USART_FLAG_ORE);
    }
	HAL_UART_IRQHandler(&uart->UartHandle);
    /* leave interrupt */
    rt_interrupt_leave();
}

void DMA1_Stream6_IRQHandler(void)
{
	HAL_DMA_IRQHandler(&uart2.UARTTxDMA_Handler);
}
void DMA1_Stream5_IRQHandler(void)
{
	HAL_DMA_IRQHandler(&uart2.UARTRxDMA_Handler);
}
#endif /* RT_USING_UART2 */

#ifdef RT_USING_UART6
/* UART3 device driver structure */
struct stm32_uart uart6 =
{
    USART6,
    USART6_IRQn,
    0,
    0,
    UART6_TX_DMA,
    UART6_RX_DMA,
    UART6_TX_DMA_IRQ,
    UART6_RX_DMA_IRQ,
	DMA_CHANNEL_5,
	DMA_CHANNEL_5,
	0,
	0
};

void USART6_IRQHandler(void)
{
    struct stm32_uart *uart;
	rt_uint32_t temp = 0;
	rt_uint32_t len = 0;
    uart = &uart6;

    /* enter interrupt */
    rt_interrupt_enter();

	if(serial6.parent.open_flag&RT_DEVICE_FLAG_DMA_RX)
	{
		if (__HAL_UART_GET_FLAG(&uart->UartHandle,UART_FLAG_IDLE) != RESET)
		{
			//清除uart总线空闲中断
			__HAL_UART_CLEAR_IDLEFLAG(&uart->UartHandle);
			temp = uart->UartHandle.Instance->SR;
			temp = uart->UartHandle.Instance->DR;
			//关闭串口DMA接收传输
			HAL_UART_RX_DMAStop(&uart->UartHandle);
			// 获取DMA中未传输的数据个数，NDTR寄存器分析见下面
			if(uart6_rx_choise)
			{
				uart6_rx_choise = 0;
				temp = uart->UARTRxDMA_Handler.Instance->NDTR;
				len = sizeof(uart6_dma_rx_buf_1) - temp;
				//切换到另外数组数据接收
				HAL_UART_Receive_DMA(&uart->UartHandle,uart6_dma_rx_buf_0,sizeof(uart6_dma_rx_buf_0));
				//将数据推入缓冲区
				rt_ringbuffer_put(&((struct rt_serial_rx_dma*) (serial6.serial_rx))->ringbuffer, uart6_dma_rx_buf_1, len);
				rt_hw_serial_isr(&serial6, RT_SERIAL_EVENT_RX_DMADONE);
			}
			else
			{
				uart6_rx_choise = 1;
				temp = uart->UARTRxDMA_Handler.Instance->NDTR;
				len = sizeof(uart6_dma_rx_buf_0) - temp;
				//切换到另外数组数据接收
				HAL_UART_Receive_DMA(&uart->UartHandle,uart6_dma_rx_buf_1,sizeof(uart6_dma_rx_buf_1));
				//将数据推入缓冲区
				rt_ringbuffer_put(&((struct rt_serial_rx_dma*) (serial6.serial_rx))->ringbuffer, uart6_dma_rx_buf_0, len);
				rt_hw_serial_isr(&serial6, RT_SERIAL_EVENT_RX_DMADONE);
			}
		}	
	}
	else
	{
		/* UART in mode Receiver ---------------------------------------------------*/
		if ((__HAL_USART_GET_FLAG(&uart->UartHandle, USART_FLAG_RXNE) != RESET) && (__HAL_UART_GET_IT_SOURCE(&uart->UartHandle, UART_IT_RXNE) != RESET))
		{
			rt_hw_serial_isr(&serial6, RT_SERIAL_EVENT_RX_IND);
			/* Clear RXNE interrupt flag */
			__HAL_USART_CLEAR_FLAG(&uart->UartHandle, USART_FLAG_RXNE);
		}
		/* UART in mode idle  ---------------------------------------------------*/
		if (__HAL_UART_GET_FLAG(&uart->UartHandle,UART_FLAG_IDLE) != RESET)
		{
			/* Clear idle interrupt flag */
			__HAL_UART_CLEAR_IDLEFLAG(&uart->UartHandle);
		}
	}
	
	
    /* UART in mode Receiver ---------------------------------------------------*/
    if ((__HAL_USART_GET_FLAG(&uart->UartHandle, USART_FLAG_RXNE) != RESET) && (__HAL_UART_GET_IT_SOURCE(&uart->UartHandle, UART_IT_RXNE) != RESET))
    {
        rt_hw_serial_isr(&serial6, RT_SERIAL_EVENT_RX_IND);
        /* Clear RXNE interrupt flag */
        __HAL_USART_CLEAR_FLAG(&uart->UartHandle, USART_FLAG_RXNE);
    }
	
	/* UART parity error interrupt occurred -------------------------------------*/
    if ((__HAL_USART_GET_FLAG(&uart->UartHandle, USART_FLAG_PE) != RESET)) {
        __HAL_USART_CLEAR_FLAG(&uart->UartHandle, USART_FLAG_PE);
    }

    /* UART frame error interrupt occurred --------------------------------------*/
    if ((__HAL_USART_GET_FLAG(&uart->UartHandle, USART_FLAG_FE) != RESET)) {
        __HAL_USART_CLEAR_FLAG(&uart->UartHandle, USART_FLAG_FE);
    }

    /* UART noise error interrupt occurred --------------------------------------*/
    if ((__HAL_USART_GET_FLAG(&uart->UartHandle, USART_FLAG_NE) != RESET)) {
        __HAL_USART_CLEAR_FLAG(&uart->UartHandle, USART_FLAG_NE);
    }

    /* UART Over-Run interrupt occurred -----------------------------------------*/
    if ((__HAL_USART_GET_FLAG(&uart->UartHandle, USART_FLAG_ORE) != RESET)) {
        __HAL_USART_CLEAR_FLAG(&uart->UartHandle, USART_FLAG_ORE);
    }
	HAL_UART_IRQHandler(&uart->UartHandle);
    /* leave interrupt */
    rt_interrupt_leave();
}

void DMA2_Stream6_IRQHandler(void)
{
	HAL_DMA_IRQHandler(&uart6.UARTTxDMA_Handler);
}

void DMA2_Stream1_IRQHandler(void)
{
	HAL_DMA_IRQHandler(&uart6.UARTRxDMA_Handler);
}

#endif /* RT_USING_UART3 */


void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
#ifdef RT_USING_UART1
	if (huart->Instance == USART1)
	{
		rt_hw_serial_isr(&serial1, RT_SERIAL_EVENT_TX_DMADONE);
	}
#endif
	
#ifdef RT_USING_UART2
	if (huart->Instance == USART2)
	{
		rt_hw_serial_isr(&serial2, RT_SERIAL_EVENT_TX_DMADONE);
	}
#endif
	
#ifdef RT_USING_UART6
	if (huart->Instance == USART6)
	{
		rt_hw_serial_isr(&serial6, RT_SERIAL_EVENT_TX_DMADONE);
	}
#endif
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	struct stm32_uart *uart = RT_NULL;
	rt_uint32_t temp = 0;
	rt_uint32_t len = 0;
#ifdef RT_USING_UART1
	if (huart->Instance == USART1)
	{
		uart = &uart1;
		temp = uart->UartHandle.Instance->SR;
		temp = uart->UartHandle.Instance->DR;
        temp = temp;
		//关闭串口DMA传输
		HAL_UART_RX_DMAStop(&uart->UartHandle);
		// 获取DMA中未传输的数据个数，NDTR寄存器分析见下面
		if(uart1_rx_choise)
		{
			uart1_rx_choise = 0;
			temp = uart->UARTRxDMA_Handler.Instance->NDTR;
			len = sizeof(uart1_dma_rx_buf_1);
			//切换到另外数组数据接收
			HAL_UART_Receive_DMA(&uart->UartHandle,uart1_dma_rx_buf_0,sizeof(uart1_dma_rx_buf_0));
			//将数据推入缓冲区
			rt_ringbuffer_put(&((struct rt_serial_rx_dma*) (serial1.serial_rx))->ringbuffer, uart1_dma_rx_buf_1, len);
			rt_hw_serial_isr(&serial1, RT_SERIAL_EVENT_RX_DMADONE);
		}
		else
		{
			uart1_rx_choise = 1;
			temp = uart->UARTRxDMA_Handler.Instance->NDTR;
			len = sizeof(uart1_dma_rx_buf_0);
			//切换到另外数组数据接收
			HAL_UART_Receive_DMA(&uart->UartHandle,uart1_dma_rx_buf_1,sizeof(uart1_dma_rx_buf_1));
			//将数据推入缓冲区
			rt_ringbuffer_put(&((struct rt_serial_rx_dma*) (serial1.serial_rx))->ringbuffer, uart1_dma_rx_buf_0, len);
			rt_hw_serial_isr(&serial1, RT_SERIAL_EVENT_RX_DMADONE);
		}
		dma_recv_len += len;
	}
#endif
	
#ifdef RT_USING_UART2
	if (huart->Instance == USART2)
	{
		uart = &uart2;
		temp = uart->UartHandle.Instance->SR;
		temp = uart->UartHandle.Instance->DR;
		//关闭串口DMA传输
		HAL_UART_RX_DMAStop(&uart->UartHandle);
		// 获取DMA中未传输的数据个数，NDTR寄存器分析见下面
		if(uart2_rx_choise)
		{
			uart2_rx_choise = 0;
			temp = uart->UARTRxDMA_Handler.Instance->NDTR;
			len = sizeof(uart2_dma_rx_buf_1);
			//切换到另外数组数据接收
			HAL_UART_Receive_DMA(&uart->UartHandle,uart2_dma_rx_buf_0,sizeof(uart2_dma_rx_buf_0));
			//将数据推入缓冲区
			rt_ringbuffer_put(&((struct rt_serial_rx_dma*) (serial2.serial_rx))->ringbuffer, uart2_dma_rx_buf_1, len);
			rt_hw_serial_isr(&serial2, RT_SERIAL_EVENT_RX_DMADONE);
		}
		else
		{
			uart2_rx_choise = 1;
			temp = uart->UARTRxDMA_Handler.Instance->NDTR;
			len = sizeof(uart2_dma_rx_buf_0);
			//切换到另外数组数据接收
			HAL_UART_Receive_DMA(&uart->UartHandle,uart2_dma_rx_buf_1,sizeof(uart2_dma_rx_buf_1));
			//将数据推入缓冲区
			rt_ringbuffer_put(&((struct rt_serial_rx_dma*) (serial2.serial_rx))->ringbuffer, uart2_dma_rx_buf_0, len);
			rt_hw_serial_isr(&serial2, RT_SERIAL_EVENT_RX_DMADONE);
		}
	}
#endif
	
#ifdef RT_USING_UART6
	if (huart->Instance == USART6)
	{
		uart = &uart6;
		temp = uart->UartHandle.Instance->SR;
		temp = uart->UartHandle.Instance->DR;
		//关闭串口DMA传输
		HAL_UART_RX_DMAStop(&uart->UartHandle);
		// 获取DMA中未传输的数据个数，NDTR寄存器分析见下面
		if(uart6_rx_choise)
		{
			uart6_rx_choise = 0;
			temp = uart->UARTRxDMA_Handler.Instance->NDTR;
			len = sizeof(uart6_dma_rx_buf_1);
			//切换到另外数组数据接收
			HAL_UART_Receive_DMA(&uart->UartHandle,uart6_dma_rx_buf_0,sizeof(uart6_dma_rx_buf_0));
			//将数据推入缓冲区
			rt_ringbuffer_put(&((struct rt_serial_rx_dma*) (serial6.serial_rx))->ringbuffer, uart6_dma_rx_buf_1, len);
			rt_hw_serial_isr(&serial6, RT_SERIAL_EVENT_RX_DMADONE);
		}
		else
		{
			uart6_rx_choise = 1;
			temp = uart->UARTRxDMA_Handler.Instance->NDTR;
			len = sizeof(uart6_dma_rx_buf_0);
			//切换到另外数组数据接收
			HAL_UART_Receive_DMA(&uart->UartHandle,uart6_dma_rx_buf_1,sizeof(uart6_dma_rx_buf_1));
			//将数据推入缓冲区
			rt_ringbuffer_put(&((struct rt_serial_rx_dma*) (serial6.serial_rx))->ringbuffer, uart6_dma_rx_buf_0, len);
			rt_hw_serial_isr(&serial6, RT_SERIAL_EVENT_RX_DMADONE);
		}
	}
#endif
}
/**
  * @brief UART MSP Initialization
  *        This function configures the hardware resources used in this example:
  *           - Peripheral's clock enable
  *           - Peripheral's GPIO Configuration
  *           - NVIC configuration for UART interrupt request enable
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef  GPIO_InitStruct;
#ifdef RT_USING_UART1
    if (huart->Instance == USART1)
    {
        /* Enable GPIO TX/RX clock */
        USART1_TX_GPIO_CLK_ENABLE();
        USART1_RX_GPIO_CLK_ENABLE();
        /* Enable USARTx clock */
        USART1_CLK_ENABLE();

        /* UART TX GPIO pin configuration  */
        GPIO_InitStruct.Pin       = USART1_TX_PIN;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_PULLUP;
        GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
        GPIO_InitStruct.Alternate = USART1_TX_AF;
        HAL_GPIO_Init(USART1_TX_GPIO_PORT, &GPIO_InitStruct);

        /* UART RX GPIO pin configuration  */
        GPIO_InitStruct.Pin = USART1_RX_PIN;
        GPIO_InitStruct.Alternate = USART1_RX_AF;
        HAL_GPIO_Init(USART1_RX_GPIO_PORT, &GPIO_InitStruct);
    }
#endif
#ifdef RT_USING_UART2
	if (huart->Instance == USART2)
    {
        /* Enable GPIO TX/RX clock */
        USART2_TX_GPIO_CLK_ENABLE();
        USART2_RX_GPIO_CLK_ENABLE();
        /* Enable USARTx clock */
        USART2_CLK_ENABLE();

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
#endif
#ifdef RT_USING_UART6
	if (huart->Instance == USART6)
    {
        /* Enable GPIO TX/RX clock */
        USART6_TX_GPIO_CLK_ENABLE();
        USART6_RX_GPIO_CLK_ENABLE();
        /* Enable USARTx clock */
        USART6_CLK_ENABLE();

        /* UART TX GPIO pin configuration  */
        GPIO_InitStruct.Pin       = USART6_TX_PIN;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_PULLUP;
        GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
        GPIO_InitStruct.Alternate = USART6_TX_AF;
        HAL_GPIO_Init(USART6_TX_GPIO_PORT, &GPIO_InitStruct);

        /* UART RX GPIO pin configuration  */
        GPIO_InitStruct.Pin = USART6_RX_PIN;
        GPIO_InitStruct.Alternate = USART6_RX_AF;
        HAL_GPIO_Init(USART6_RX_GPIO_PORT, &GPIO_InitStruct);
    }
#endif
}

/**
  * @brief UART MSP De-Initialization
  *        This function frees the hardware resources used in this example:
  *          - Disable the Peripheral's clock
  *          - Revert GPIO and NVIC configuration to their default state
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
#ifdef RT_USING_UART1
    if (huart->Instance == USART1)
    {
        /* Reset peripherals */
        USART1_FORCE_RESET();
        USART1_RELEASE_RESET();

        /* Disable peripherals and GPIO Clocks */
        /* Configure UART Tx as alternate function  */
        HAL_GPIO_DeInit(USART1_TX_GPIO_PORT, USART1_TX_PIN);
        /* Configure UART Rx as alternate function  */
        HAL_GPIO_DeInit(USART1_RX_GPIO_PORT, USART1_RX_PIN);

        /* Disable the NVIC for UART */
        HAL_NVIC_DisableIRQ(USART1_IRQn);
    }
#endif
#ifdef RT_USING_UART2	
	if (huart->Instance == USART2)
    {
        /* Reset peripherals */
        USART2_FORCE_RESET();
        USART2_RELEASE_RESET();

        /* Disable peripherals and GPIO Clocks */
        /* Configure UART Tx as alternate function  */
        HAL_GPIO_DeInit(USART2_TX_GPIO_PORT, USART2_TX_PIN);
        /* Configure UART Rx as alternate function  */
        HAL_GPIO_DeInit(USART2_RX_GPIO_PORT, USART2_RX_PIN);

        /* Disable the NVIC for UART */
        HAL_NVIC_DisableIRQ(USART2_IRQn);
    }
#endif
#ifdef RT_USING_UART6	
	if (huart->Instance == USART6)
    {
        /* Reset peripherals */
        USART6_FORCE_RESET();
        USART6_RELEASE_RESET();

        /* Disable peripherals and GPIO Clocks */
        /* Configure UART Tx as alternate function  */
        HAL_GPIO_DeInit(USART6_TX_GPIO_PORT, USART6_TX_PIN);
        /* Configure UART Rx as alternate function  */
        HAL_GPIO_DeInit(USART6_RX_GPIO_PORT, USART6_RX_PIN);

        /* Disable the NVIC for UART */
        HAL_NVIC_DisableIRQ(USART6_IRQn);
    }
#endif
}

static void NVIC_Configuration(struct stm32_uart *uart)
{
	struct rt_serial_device *serial = RT_NULL;
	serial = (struct rt_serial_device *)uart->user_data;

    HAL_NVIC_SetPriority(uart->irq, 0, 1);
	HAL_NVIC_EnableIRQ(uart->irq);
	if(serial->parent.flag & RT_DEVICE_FLAG_DMA_TX)
	{
		HAL_NVIC_SetPriority(uart->dma_tx_irq, 1, 1);
		HAL_NVIC_EnableIRQ(uart->dma_tx_irq);
	}
	if(serial->parent.flag & RT_DEVICE_FLAG_DMA_RX)
	{
		HAL_NVIC_SetPriority(uart->dma_rx_irq, 1, 0);
		HAL_NVIC_EnableIRQ(uart->dma_rx_irq);
	}
}

static void DMA_Configuration(struct stm32_uart *uart)
{
	struct rt_serial_device *serial = RT_NULL;
	serial = (struct rt_serial_device *)uart->user_data;
	if(serial->parent.flag & RT_DEVICE_FLAG_DMA_TX)
	{
		__HAL_RCC_DMA1_CLK_ENABLE();
		__HAL_RCC_DMA2_CLK_ENABLE();
		__HAL_LINKDMA(&uart->UartHandle,hdmatx,uart->UARTTxDMA_Handler);
		
		uart->UARTTxDMA_Handler.Instance=uart->tx_stream_channel;
		uart->UARTTxDMA_Handler.Init.Channel=uart->tx_dma_channel;
		uart->UARTTxDMA_Handler.Init.Direction=DMA_MEMORY_TO_PERIPH;
		uart->UARTTxDMA_Handler.Init.PeriphInc=DMA_PINC_DISABLE;
		uart->UARTTxDMA_Handler.Init.MemInc=DMA_MINC_ENABLE;
		uart->UARTTxDMA_Handler.Init.PeriphDataAlignment=DMA_PDATAALIGN_BYTE;
		uart->UARTTxDMA_Handler.Init.MemDataAlignment=DMA_MDATAALIGN_BYTE;
		uart->UARTTxDMA_Handler.Init.Mode=DMA_NORMAL;
		uart->UARTTxDMA_Handler.Init.Priority=DMA_PRIORITY_HIGH;
		uart->UARTTxDMA_Handler.Init.FIFOMode=DMA_FIFOMODE_DISABLE; 
		uart->UARTTxDMA_Handler.Init.FIFOThreshold=DMA_FIFO_THRESHOLD_FULL; 
		uart->UARTTxDMA_Handler.Init.MemBurst=DMA_MBURST_SINGLE;
		uart->UARTTxDMA_Handler.Init.PeriphBurst=DMA_PBURST_SINGLE;

		HAL_DMA_DeInit(&uart->UARTTxDMA_Handler);   
		HAL_DMA_Init(&uart->UARTTxDMA_Handler);
	}
	if(serial->parent.flag & RT_DEVICE_FLAG_DMA_RX)
	{
		__HAL_RCC_DMA1_CLK_ENABLE();
		__HAL_RCC_DMA2_CLK_ENABLE();
		__HAL_LINKDMA(&uart->UartHandle,hdmarx,uart->UARTRxDMA_Handler);
		
		uart->UARTRxDMA_Handler.Instance = uart->rx_stream_channel;
		uart->UARTRxDMA_Handler.Init.Channel = uart->rx_dma_channel;
		uart->UARTRxDMA_Handler.Init.Direction = DMA_PERIPH_TO_MEMORY;
		uart->UARTRxDMA_Handler.Init.PeriphInc = DMA_PINC_DISABLE;
		uart->UARTRxDMA_Handler.Init.MemInc = DMA_MINC_ENABLE;
		uart->UARTRxDMA_Handler.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		uart->UARTRxDMA_Handler.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
		uart->UARTRxDMA_Handler.Init.Mode = DMA_CIRCULAR;
		uart->UARTRxDMA_Handler.Init.Priority = DMA_PRIORITY_VERY_HIGH;
		uart->UARTRxDMA_Handler.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
		
		HAL_DMA_DeInit(&uart->UARTRxDMA_Handler);   
		HAL_DMA_Init(&uart->UARTRxDMA_Handler);
	}
}

int stm32_hw_usart_init(void)
{
    struct stm32_uart *uart;
#ifdef RT_USING_UART1
	struct serial_configure uart1_config = RT_SERIAL_CONFIG_DEFAULT;
    uart = &uart1;
    uart->UartHandle.Instance = USART1;
	uart->user_data = &serial1;
    serial1.ops    = &stm32_uart_ops;
    serial1.config = uart1_config;
	serial1.config.bufsz = (1024*10);
	serial1.config.baud_rate = BAUD_RATE_921600;
    /* register UART1 device */
    rt_hw_serial_register(&serial1, "uart1",
                          RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_DMA_RX | RT_DEVICE_FLAG_DMA_TX,
                          uart);
	DMA_Configuration(uart);
	NVIC_Configuration(uart);
#endif /* RT_USING_UART1 */

#ifdef RT_USING_UART2
	struct serial_configure uart2_config = RT_SERIAL_CONFIG_DEFAULT;
    uart = &uart2;
    uart->UartHandle.Instance = USART2;
	uart->user_data = &serial2;
    serial2.ops    = &stm32_uart_ops;
    serial2.config = uart2_config;
	serial2.config.bufsz = (1024);
	serial2.config.baud_rate = BAUD_RATE_115200;
    /* register UART2 device */
    rt_hw_serial_register(&serial2, "uart2",
                          RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_STREAM,
                          uart);
	DMA_Configuration(uart);
	NVIC_Configuration(uart);
#endif /* RT_USING_UART2 */

#ifdef RT_USING_UART6
	struct serial_configure uart6_config = RT_SERIAL_CONFIG_DEFAULT;
    uart = &uart6;
    uart->UartHandle.Instance = USART6;
	uart->user_data = &serial6;
    serial6.ops    = &stm32_uart_ops;
    serial6.config = uart6_config;
	serial6.config.bufsz = (1024*6);
	serial6.config.baud_rate = BAUD_RATE_115200;
    /* register UART3 device */
    rt_hw_serial_register(&serial6, "uart6",
                          RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_DMA_RX | RT_DEVICE_FLAG_DMA_TX,
                          uart);
	DMA_Configuration(uart);
	NVIC_Configuration(uart);
#endif /* RT_USING_UART6 */

    return 0;
}
