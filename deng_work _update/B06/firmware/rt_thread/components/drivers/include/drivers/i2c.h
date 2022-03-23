/*
 * File      : i2c.h
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006 - 2012, RT-Thread Development Team
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
 * Date           Author        Notes
 * 2012-04-25     weety         first version
 */

#ifndef __I2C_H__
#define __I2C_H__

#include <rtthread.h>
#include "stm32f4xx.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RT_I2C_WR                0x0000
#define RT_I2C_RD               (1u << 0)
#define RT_I2C_ADDR_10BIT       (1u << 2)  /* this is a ten bit chip address */
#define RT_I2C_NO_START         (1u << 4)
#define RT_I2C_IGNORE_NACK      (1u << 5)
#define RT_I2C_NO_READ_ACK      (1u << 6)  /* when I2C reading, we do not ACK */


#define I2C_CLK_SPEED_400KBPS	0x00401959
#define I2C_CLK_SPEED_200KBPS	0x004019D6
#define I2C_CLK_SPEED_100KBPS	0x00C0EAFF
#define I2C_CLK_SPEED_50KBPS	0x1060EFFF

#define RT_I2C_CONFIG_DEFAULT	\
{								\
	I2C_CLK_SPEED_50KBPS,		\
	0x00,						\
	I2C_ADDRESSINGMODE_7BIT,	\
	I2C_DUALADDRESS_DISABLE,	\
	0x00,						\
	I2C_OA2_NOMASK,				\
	I2C_GENERALCALL_DISABLE,	\
	I2C_NOSTRETCH_DISABLE		\
}
struct i2c_configure
{
	rt_uint32_t Timing;              /*!< Specifies the I2C_TIMINGR_register value.
                                  This parameter calculated by referring to I2C initialization
//                                         section in Reference manual */

  rt_uint32_t OwnAddress1;         /*!< Specifies the first device own address.
                                  This parameter can be a 7-bit or 10-bit address. */

  rt_uint32_t AddressingMode;      /*!< Specifies if 7-bit or 10-bit addressing mode is selected.
                                  This parameter can be a value of @ref I2C_ADDRESSING_MODE */

  rt_uint32_t DualAddressMode;     /*!< Specifies if dual addressing mode is selected.
                                  This parameter can be a value of @ref I2C_DUAL_ADDRESSING_MODE */

  rt_uint32_t OwnAddress2;         /*!< Specifies the second device own address if dual addressing mode is selected
                                  This parameter can be a 7-bit address. */

  rt_uint32_t OwnAddress2Masks;    /*!< Specifies the acknowledge mask address second device own address if dual addressing mode is selected
                                  This parameter can be a value of @ref I2C_OWN_ADDRESS2_MASKS */

  rt_uint32_t GeneralCallMode;     /*!< Specifies if general call mode is selected.
                                  This parameter can be a value of @ref I2C_GENERAL_CALL_ADDRESSING_MODE */

  rt_uint32_t NoStretchMode;       /*!< Specifies if nostretch mode is selected.
                                  This parameter can be a value of @ref I2C_NOSTRETCH_MODE */
};

struct stm32_i2c
{
	I2C_TypeDef*	i2c_device;
	IRQn_Type		irq_ev;
	IRQn_Type		irq_er;
	rt_uint8_t		irq_pre_pri;
	rt_uint8_t		irq_sub_pri;
    IRQn_Type		dma_tx_irq;
    IRQn_Type		dma_rx_irq;
	I2C_HandleTypeDef  I2C_Handler;
};


#ifdef RT_USING_I2C_INT_TRANSFER

typedef enum i2c_rw_option
{
    RW_BLOCKED = 0,
    RW_INT     = 1
}i2c_rw_option_t;

struct rt_i2c_msg
{
    rt_uint8_t      dev_addr;
    rt_uint8_t      reg_addr;
    rt_uint8_t      write_len;
    rt_uint8_t      read_len;
    rt_uint8_t      *buf;
	rt_uint8_t      runflag;
    i2c_rw_option_t rw_option; 
};
#else
struct rt_i2c_msg
{
    rt_uint16_t addr;
    rt_uint16_t flags;
    rt_uint16_t len;
    rt_uint8_t  *buf;
};
#endif


struct rt_i2c_bus_device;

struct rt_i2c_bus_device_ops
{
    rt_err_t (*configure)(struct rt_i2c_bus_device *i2c_bus, 
						  struct i2c_configure *cfg);
    rt_err_t (*control)(struct rt_i2c_bus_device *i2c_bus, 
						int cmd, 
						void *arg);
    rt_size_t (*master_xfer)(struct rt_i2c_bus_device *bus,
                             struct rt_i2c_msg msgs[],
                             rt_uint32_t num);
    rt_size_t (*slave_xfer)(struct rt_i2c_bus_device *bus,
                            struct rt_i2c_msg msgs[],
                            rt_uint32_t num);
    rt_err_t (*i2c_bus_control)(struct rt_i2c_bus_device *bus,
                                rt_uint32_t,
                                rt_uint32_t);
};

/*for i2c bus driver*/
struct rt_i2c_bus_device
{
	struct rt_device 	   parent;
	struct i2c_configure   config;
	struct stm32_i2c	   *dev;
	const struct rt_i2c_bus_device_ops *ops;
	rt_uint16_t  flags;
	rt_uint16_t  addr;
    rt_bool_t    bus_err;
	struct rt_i2c_msg   i2c_int_msg;
	struct rt_mutex lock;
	rt_uint32_t  timeout;
	rt_uint32_t  retries;
	void *priv;
};


#ifdef RT_I2C_DEBUG
#define i2c_dbg(fmt, ...)   rt_kprintf(fmt, ##__VA_ARGS__)
#else
#define i2c_dbg(fmt, ...)
#endif

struct rt_i2c_bus_device *rt_i2c_bus_device_find(const char *bus_name);
rt_err_t rt_i2c_bus_device_register(struct rt_i2c_bus_device *bus,
                                    const char               *bus_name);
#ifdef RT_USING_I2C_INT_TRANSFER
rt_err_t rt_i2c_master_send(struct rt_i2c_bus_device *bus,
                             rt_uint8_t               dev_addr,
                             rt_uint8_t               reg_addr,
                             const rt_uint8_t         *buf,
                             rt_uint8_t               write_len,
                             i2c_rw_option_t          write_option);
rt_err_t rt_i2c_master_recv(struct rt_i2c_bus_device *bus,
                             rt_uint8_t               dev_addr,
                             rt_uint8_t               reg_addr,
                             rt_uint8_t               *buf,
                             rt_uint8_t               read_len,
                             i2c_rw_option_t          read_option);
#else
rt_size_t rt_i2c_transfer(struct rt_i2c_bus_device *bus,
                          struct rt_i2c_msg         msgs[],
                          rt_uint32_t               num);
rt_size_t rt_i2c_master_send(struct rt_i2c_bus_device *bus,
                             rt_uint16_t               addr,
                             rt_uint16_t               flags,
                             const rt_uint8_t         *buf,
                             rt_uint32_t               count);
rt_size_t rt_i2c_master_recv(struct rt_i2c_bus_device *bus,
                             rt_uint16_t               addr,
                             rt_uint16_t               flags,
                             rt_uint8_t               *buf,
                             rt_uint32_t               count);
#endif
int rt_i2c_core_init(void);

#ifdef __cplusplus
}
#endif

#endif
