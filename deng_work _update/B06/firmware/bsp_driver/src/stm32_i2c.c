#include <rtthread.h>
#include <board.h>
#include <rtdevice.h>
#include "i2c.h"
#include "stdio.h"
#include <stdbool.h>
#include "hrt.h"

/* Definition for I2C1: I2C1_SCL-PB6 I2C1_SDA-PB7  */
#define I2C1_CLK_ENABLE()                __HAL_RCC_I2C1_CLK_ENABLE()
#define I2C1_DMA_CLK_ENABLE()            __HAL_RCC_DMA1_CLK_ENABLE()
#define I2C1_SDA_GPIO_CLK_ENABLE()       __HAL_RCC_GPIOB_CLK_ENABLE()
#define I2C1_SCL_GPIO_CLK_ENABLE()       __HAL_RCC_GPIOB_CLK_ENABLE()

#define I2C1_FORCE_RESET()               __HAL_RCC_I2C1_FORCE_RESET()
#define I2C1_RELEASE_RESET()             __HAL_RCC_I2C1_RELEASE_RESET()

/* Definition for I2Cx Pins */
#define I2C1_SCL_PIN                    GPIO_PIN_6
#define I2C1_SCL_GPIO_PORT              GPIOB
#define I2C1_SCL_AF                     GPIO_AF4_I2C1
#define I2C1_SDA_PIN                    GPIO_PIN_7
#define I2C1_SDA_GPIO_PORT              GPIOB
#define I2C1_SDA_AF                     GPIO_AF4_I2C1

/* Definition for I2Cx's NVIC */
#define I2C1_EV_IRQN                    I2C1_EV_IRQn
#define I2C1_ER_IRQN                    I2C1_ER_IRQn
#define I2C1_EV_IRQHANDLER              I2C1_EV_IRQHandler
#define I2C1_ER_IRQHANDLER              I2C1_ER_IRQHandler

/* Definition for I2Cx's DMA */
#define I2C1_TX_DMA_CHANNEL             DMA_CHANNEL_7
#define I2C1_RX_DMA_CHANNEL             DMA_CHANNEL_0
#define I2C1_DMA_INSTANCE_TX            DMA1_Stream7
#define I2C1_DMA_INSTANCE_RX            DMA1_Stream0

/* Definition for I2Cx's NVIC */
#define I2C1_DMA_TX_IRQn                DMA1_Stream7_IRQn
#define I2C1_DMA_RX_IRQn                DMA1_Stream0_IRQn
#define I2C1_DMA_TX_IRQHandler          DMA1_Stream7_IRQHandler
#define I2C1_DMA_RX_IRQHandler          DMA1_Stream0_IRQHandler

#define I2C1_SCL_SDA_AF                 GPIO_AF4_I2C1
#define RCC_PERIP_HCLK_I2C1             RCC_PERIPHCLK_I2C1
#define RCC_I2C1_CLKSOURCE_SYSCLK       RCC_I2C1CLKSOURCE_PCLK1


struct rt_i2c_bus_device i2c1_bus;

static rt_err_t stm32_i2c_configure(struct rt_i2c_bus_device *i2c_bus, struct i2c_configure *cfg)
{
    struct stm32_i2c *i2c;
	
    RT_ASSERT(i2c_bus != RT_NULL);
    RT_ASSERT(cfg != RT_NULL);

    i2c = (struct stm32_i2c *)i2c_bus->dev;
	i2c->I2C_Handler.Instance = i2c->i2c_device;

	i2c->I2C_Handler.Init.ClockSpeed      = cfg->ClockSpeed;
	i2c->I2C_Handler.Init.DutyCycle		  = cfg->DutyCycle;
	i2c->I2C_Handler.Init.OwnAddress1     = cfg->OwnAddress1;
	i2c->I2C_Handler.Init.AddressingMode  = cfg->AddressingMode;
	i2c->I2C_Handler.Init.OwnAddress2     = cfg->OwnAddress2;
	i2c->I2C_Handler.Init.DualAddressMode = cfg->DualAddressMode;
	i2c->I2C_Handler.Init.GeneralCallMode = cfg->GeneralCallMode;
	i2c->I2C_Handler.Init.NoStretchMode   = cfg->NoStretchMode;
	HAL_I2C_Init(&i2c->I2C_Handler);
	return RT_EOK;
}

static rt_err_t stm32_i2c_control(struct rt_i2c_bus_device *i2c_bus, int cmd, void *arg)
{
    struct stm32_i2c *i2c;
    RT_ASSERT(i2c_bus != RT_NULL);
    i2c = (struct stm32_i2c *)i2c_bus->dev;

    switch (cmd)
    {
        case RT_DEVICE_CTRL_CLR_INT:
            /* disable rx irq */
            HAL_NVIC_DisableIRQ(i2c->irq_ev);
			HAL_NVIC_DisableIRQ(i2c->irq_er);
            break;
        case RT_DEVICE_CTRL_SET_INT:
			HAL_NVIC_EnableIRQ(i2c->irq_ev);
			HAL_NVIC_EnableIRQ(i2c->irq_er);
            break;
    }
    return RT_EOK;
}


static const struct rt_i2c_bus_device_ops stm32_i2c_ops =
{
    stm32_i2c_configure,
    stm32_i2c_control,
    RT_NULL,
    RT_NULL,
    RT_NULL
};

/* I2C2 device driver structure */
struct stm32_i2c i2c1 =
{
    I2C1,
    I2C1_EV_IRQn,
    I2C1_ER_IRQn,
    1,
    1,
    DMA1_Stream7_IRQn,
    DMA1_Stream0_IRQn
};
uint16_t i2c1_ev_cnt = 0;
void I2C1_EV_IRQHandler(void)
{
	/* enter interrupt */
    rt_interrupt_enter();
	HAL_I2C_EV_IRQHandler(&i2c1.I2C_Handler);
	i2c1_ev_cnt++;
    /* leave interrupt */
    rt_interrupt_leave();
}

uint16_t i2c1_dead_cnt = 0;
void I2C1_ER_IRQHandler(void)
{
	static uint32_t last_haltick = 0;
	static uint8_t error_num = 0;
	/* enter interrupt */
    rt_interrupt_enter();
	HAL_I2C_ER_IRQHandler(&i2c1.I2C_Handler);
	if (HAL_GetTick() == last_haltick)
	{
		if (error_num++ >= 5)
		{
			error_num = 0;
			i2c1_dead_cnt++;
		}
	}
	else
	{
		error_num = 0;
	}
		
	last_haltick = HAL_GetTick();
    /* leave interrupt */
    rt_interrupt_leave();
}

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
	GPIO_InitTypeDef  GPIO_InitStruct;
	if (hi2c->Instance == I2C1)
	{		
		//使能时钟
		I2C1_SCL_GPIO_CLK_ENABLE();
		I2C1_SDA_GPIO_CLK_ENABLE();
		I2C1_CLK_ENABLE(); 
		
		//拉高SCL
		GPIO_InitStruct.Pin = I2C1_SCL_PIN;      
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;   
		GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;         
		HAL_GPIO_Init(I2C1_SCL_GPIO_PORT,&GPIO_InitStruct);
		HAL_GPIO_WritePin(I2C1_SCL_GPIO_PORT, I2C1_SCL_PIN, GPIO_PIN_SET);

		//拉高SDA
		GPIO_InitStruct.Pin = I2C1_SDA_PIN;              
		HAL_GPIO_Init(I2C1_SDA_GPIO_PORT,&GPIO_InitStruct);
		HAL_GPIO_WritePin(I2C1_SDA_GPIO_PORT, I2C1_SDA_PIN, GPIO_PIN_SET);             	 

		//复位I2C控制器
		hi2c->Instance->CR1= I2C_CR1_SWRST;         
		hi2c->Instance->CR1= 0;
		
		//重新配置I2C
		GPIO_InitStruct.Pin       = I2C1_SCL_PIN;
		GPIO_InitStruct.Mode      = GPIO_MODE_AF_OD;
		GPIO_InitStruct.Pull      = GPIO_PULLUP;
		GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
		GPIO_InitStruct.Alternate = I2C1_SCL_SDA_AF;
		HAL_GPIO_Init(I2C1_SCL_GPIO_PORT, &GPIO_InitStruct);

		GPIO_InitStruct.Pin       = I2C1_SDA_PIN;
		GPIO_InitStruct.Alternate = I2C1_SCL_SDA_AF;
		HAL_GPIO_Init(I2C1_SDA_GPIO_PORT, &GPIO_InitStruct);
		
		I2C1_FORCE_RESET();
		I2C1_RELEASE_RESET();
	}
}

static void I2C_NVIC_Configuration(struct stm32_i2c *i2c)
{
	if (i2c->I2C_Handler.Instance == I2C1 || i2c->I2C_Handler.Instance == I2C2)
	{
		HAL_NVIC_SetPriority(i2c->irq_ev, i2c->irq_pre_pri, i2c->irq_sub_pri);
		HAL_NVIC_SetPriority(i2c->irq_er, i2c->irq_pre_pri, i2c->irq_sub_pri);
	}
}


void HAL_I2C_MspDeInit(I2C_HandleTypeDef *hi2c)
{
	if (hi2c->Instance == I2C1)
	{
		I2C1_FORCE_RESET();
		I2C1_RELEASE_RESET();

		HAL_GPIO_DeInit(I2C1_SCL_GPIO_PORT, I2C1_SCL_PIN);
		HAL_GPIO_DeInit(I2C1_SDA_GPIO_PORT, I2C1_SDA_PIN);

		HAL_NVIC_DisableIRQ(I2C1_ER_IRQN);
		HAL_NVIC_DisableIRQ(I2C1_EV_IRQN);
	}
}

/* I2C2 init function */
void MX_I2C_Init(I2C_TypeDef *i2c_device)
{
	if (i2c_device == I2C1)
	{
		i2c1.I2C_Handler.Instance = I2C1;  
		i2c1.I2C_Handler.Init.ClockSpeed = 20000;
		i2c1.I2C_Handler.Init.DutyCycle = I2C_DUTYCYCLE_2;
		i2c1.I2C_Handler.Init.OwnAddress1 = 0;
		i2c1.I2C_Handler.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
		i2c1.I2C_Handler.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
		i2c1.I2C_Handler.Init.OwnAddress2 = 0;
		i2c1.I2C_Handler.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
		i2c1.I2C_Handler.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
		if (HAL_I2C_Init(&i2c1.I2C_Handler) != HAL_OK)
		{
			// _Error_Handler(__FILE__, __LINE__);
		}
	}
}

static void I2C_bus_clear(void)
{
    uint8_t pinSDA = 1;
    GPIO_InitTypeDef  GPIO_InitStructure;
	
    GPIO_InitStructure.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStructure.Pull  = GPIO_NOPULL;
    GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStructure.Pin = I2C1_SDA_PIN;
    HAL_GPIO_Init(I2C1_SDA_GPIO_PORT, &GPIO_InitStructure);
	
    GPIO_InitStructure.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStructure.Pin = I2C1_SCL_PIN;
    HAL_GPIO_Init(I2C1_SCL_GPIO_PORT, &GPIO_InitStructure);

    //if(pinSDA == 0) whether sda is high or low,give pulse both will make device more robust
    {
        uint8_t i;
        for(i = 0; i < 10; i++)
        {
            HAL_GPIO_WritePin(I2C1_SCL_GPIO_PORT, I2C1_SCL_PIN, GPIO_PIN_RESET);
            hrt_delay_us(10);
            HAL_GPIO_WritePin(I2C1_SCL_GPIO_PORT, I2C1_SCL_PIN, GPIO_PIN_SET);
            hrt_delay_us(10);
        }

        pinSDA = HAL_GPIO_ReadPin(I2C1_SDA_GPIO_PORT, I2C1_SDA_PIN);
        printf("[I2C BUS CLEAR]after SCL output 10 pules pinSDA:%d\n", pinSDA);
    }
}

void rt_hw_i2c_init(void)
{
    struct i2c_configure config = RT_I2C_CONFIG_DEFAULT;
	I2C_bus_clear();
    i2c1_bus.dev    = &i2c1;
    i2c1_bus.ops    = &stm32_i2c_ops;
    i2c1_bus.config = config;
    i2c1_bus.bus_err = false;
	
	MX_I2C_Init(i2c1.i2c_device);
    I2C_NVIC_Configuration(&i2c1);
    rt_i2c_bus_device_register(&i2c1_bus, "i2c1");
}

