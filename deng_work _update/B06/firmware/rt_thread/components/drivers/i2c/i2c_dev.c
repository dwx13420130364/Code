/*
 * File      : i2c_dev.c
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
 * 2014-08-03     bernard       fix some compiling warning
 * 2015-11-06     Yang Yang     impliment of hardware i2c component
 */

#include <rtdevice.h>
#include "i2c.h"
#include "i2c_dev.h"

static rt_err_t i2c_bus_device_init(struct rt_device *dev)
{
    rt_err_t result = RT_EOK;
    struct rt_i2c_bus_device *i2c_bus;

    RT_ASSERT(dev != RT_NULL);
    i2c_bus = (struct rt_i2c_bus_device *)dev;

    /* initialize rx/tx */

    /* apply configuration */
    if (i2c_bus->ops->configure)
        result = i2c_bus->ops->configure(i2c_bus, &i2c_bus->config);

    return result;
}

static rt_err_t i2c_bus_device_open(struct rt_device *dev, rt_uint16_t oflag)
{
    struct rt_i2c_bus_device *i2c_bus;

    RT_ASSERT(dev != RT_NULL);
    i2c_bus = (struct rt_i2c_bus_device *)dev;

    /* check device flag with the open flag */
    if ((oflag & RT_DEVICE_FLAG_DMA_RX) && !(dev->flag & RT_DEVICE_FLAG_DMA_RX))
        return -RT_EIO;
    if ((oflag & RT_DEVICE_FLAG_DMA_TX) && !(dev->flag & RT_DEVICE_FLAG_DMA_TX))
        return -RT_EIO;
    if ((oflag & RT_DEVICE_FLAG_INT_RX) && !(dev->flag & RT_DEVICE_FLAG_INT_RX))
        return -RT_EIO;
    if ((oflag & RT_DEVICE_FLAG_INT_TX) && !(dev->flag & RT_DEVICE_FLAG_INT_TX))
        return -RT_EIO;

    /* get open flags */
    dev->open_flag = oflag & 0xff;

    /* configure low level device */
    /* initialize the Rx/Tx structure according to open flag */
    //if (i2c_bus->parent->read == RT_NULL)
    {
        if (oflag & RT_DEVICE_FLAG_DMA_RX)
        {
            dev->open_flag |= RT_DEVICE_FLAG_DMA_RX;
        }
        else if (oflag & RT_DEVICE_FLAG_INT_RX)
        {
            dev->open_flag |= RT_DEVICE_FLAG_INT_RX;
            /* configure low level device */
            i2c_bus->ops->control(i2c_bus, RT_DEVICE_CTRL_SET_INT, (void *)RT_DEVICE_FLAG_INT_RX);
        }
        else
        {
        }
    }

    //if (serial->serial_tx == RT_NULL)
    {
        if (oflag & RT_DEVICE_FLAG_DMA_TX)
        {
            dev->open_flag |= RT_DEVICE_FLAG_DMA_TX;
        }
        else if (oflag & RT_DEVICE_FLAG_INT_TX)
        {
            dev->open_flag |= RT_DEVICE_FLAG_INT_TX;
            /* configure low level device */
            i2c_bus->ops->control(i2c_bus, RT_DEVICE_CTRL_SET_INT, (void *)RT_DEVICE_FLAG_INT_TX);
        }
        else
        {
        }
    }

    return RT_EOK;
}

static rt_err_t i2c_bus_device_close(struct rt_device *dev)
{
    struct rt_i2c_bus_device *i2c_bus;

    RT_ASSERT(dev != RT_NULL);
    i2c_bus = (struct rt_i2c_bus_device *)dev;

    /* configure low level device */
    i2c_bus->ops->control(i2c_bus, RT_DEVICE_CTRL_CLR_INT, (void *)RT_DEVICE_FLAG_INT_TX);
    /* configure low level device */
    i2c_bus->ops->control(i2c_bus, RT_DEVICE_CTRL_CLR_INT, (void *)RT_DEVICE_FLAG_INT_RX);

    return RT_EOK;
}


#ifdef RT_USING_I2C_INT_TRANSFER
static rt_size_t i2c_bus_device_read(struct rt_device *dev, rt_off_t pos, void *buf,rt_size_t read_len, ...)
{
    va_list args;
    struct rt_i2c_bus_device *bus = (struct rt_i2c_bus_device *)dev->user_data;

    RT_ASSERT(bus != RT_NULL);
    RT_ASSERT(buf != RT_NULL);

    va_start(args, read_len);
    va_list args_new = va_arg(args,va_list);
    rt_uint32_t     dev_addr    = va_arg(args_new,rt_uint32_t);
    rt_uint32_t     reg_addr    = va_arg(args_new,rt_uint32_t);
    i2c_rw_option_t read_option = (i2c_rw_option_t)va_arg(args_new,rt_uint32_t);
    va_end(args);
    //i2c_dbg("I2C bus dev [%s],dev_addr = 0x%x,reg_addr = 0x%x,reading %u bytes.\n", dev->parent.name, (rt_uint8_t)dev_addr, (rt_uint8_t)reg_addr,read_len);
    if(dev->open_flag & RT_DEVICE_FLAG_INT_RX)
    {
        return rt_i2c_master_recv(bus, (rt_uint8_t)dev_addr, (rt_uint8_t)reg_addr, (rt_uint8_t *)buf,(rt_uint8_t)read_len,read_option);
    }
    else
    {
        return 0;
    }
}

static rt_size_t i2c_bus_device_write(struct rt_device *dev, rt_off_t pos, const void *buf, rt_size_t write_len, ...)
{
    va_list args;
    struct rt_i2c_bus_device *bus = (struct rt_i2c_bus_device *)dev->user_data;

    RT_ASSERT(bus != RT_NULL);
    RT_ASSERT(buf != RT_NULL);
    
    va_start(args, write_len);
    va_list args_new = va_arg(args,va_list);
    rt_uint32_t     dev_addr     = va_arg(args_new,rt_uint32_t);
    rt_uint32_t     reg_addr     = va_arg(args_new,rt_uint32_t);
    i2c_rw_option_t write_option = (i2c_rw_option_t)va_arg(args_new,rt_uint32_t);
    va_end(args);
    //i2c_dbg("I2C bus dev [%s],dev_addr = 0x%x,reg_addr = 0x%x,reading %u bytes.\n", dev->parent.name, (rt_uint8_t)dev_addr, (rt_uint8_t)reg_addr,write_len);
    if(dev->open_flag & RT_DEVICE_FLAG_INT_TX)
    {
        return rt_i2c_master_send(bus, (rt_uint8_t)dev_addr, (rt_uint8_t)reg_addr, (const rt_uint8_t *)buf,(rt_uint8_t)write_len,write_option);
    }
    else
    {
        return 0;
    }
}

static rt_err_t i2c_bus_device_control(struct rt_device * dev,
                                       rt_uint8_t  cmd,
                                       void       *args)
{
    struct rt_i2c_bus_device *bus = (struct rt_i2c_bus_device *)dev->user_data;

    RT_ASSERT(bus != RT_NULL);

    switch (cmd)
    {
        case RT_DEVICE_CTRL_CONFIG:
            /* configure device */
            bus->ops->configure(bus, (struct i2c_configure *)args);
            break;

        default :
            /* control device */
            bus->ops->control(bus, cmd, args);
            break;
    }

    return RT_EOK;
}
#else
static rt_size_t i2c_bus_device_read(rt_device_t dev,
                                     rt_off_t    pos,
                                     void       *buffer,
                                     rt_size_t   count)
{
    rt_uint16_t addr;
    rt_uint16_t flags;
    struct rt_i2c_bus_device *bus = (struct rt_i2c_bus_device *)dev->user_data;

    RT_ASSERT(bus != RT_NULL);
    RT_ASSERT(buffer != RT_NULL);

    i2c_dbg("I2C bus dev [%s] reading %u bytes.\n", dev->parent.name, count);

    addr = pos & 0xffff;
    flags = (pos >> 16) & 0xffff;

    return rt_i2c_master_recv(bus, addr, flags, buffer, count);
}

static rt_size_t i2c_bus_device_write(rt_device_t dev,
                                      rt_off_t    pos,
                                      const void *buffer,
                                      rt_size_t   count)
{
    rt_uint16_t addr;
    rt_uint16_t flags;
    struct rt_i2c_bus_device *bus = (struct rt_i2c_bus_device *)dev->user_data;

    RT_ASSERT(bus != RT_NULL);
    RT_ASSERT(buffer != RT_NULL);

    i2c_dbg("I2C bus dev [%s] writing %u bytes.\n", dev->parent.name, count);

    addr = pos & 0xffff;
    flags = (pos >> 16) & 0xffff;

    return rt_i2c_master_send(bus, addr, flags, buffer, count);
}

static rt_err_t i2c_bus_device_control(rt_device_t dev,
                                       rt_uint8_t  cmd,
                                       void       *args)
{
    rt_err_t ret;
    struct rt_i2c_priv_data *priv_data;
    struct rt_i2c_bus_device *bus = (struct rt_i2c_bus_device *)dev->user_data;

    RT_ASSERT(bus != RT_NULL);

    switch (cmd)
    {
        /* set 10-bit addr mode */
        case RT_I2C_DEV_CTRL_10BIT:
            bus->flags |= RT_I2C_ADDR_10BIT;
            break;
        case RT_I2C_DEV_CTRL_ADDR:
            bus->addr = *(rt_uint16_t *)args;
            break;
        case RT_I2C_DEV_CTRL_TIMEOUT:
            bus->timeout = *(rt_uint32_t *)args;
            break;
        case RT_I2C_DEV_CTRL_RW:
            priv_data = (struct rt_i2c_priv_data *)args;
            ret = rt_i2c_transfer(bus, priv_data->msgs, priv_data->number);
            if (ret < 0)
            {
                return -RT_EIO;
            }
            break;
        default:
            break;
    }

    return RT_EOK;
}
#endif



rt_err_t rt_i2c_bus_device_device_init(struct rt_i2c_bus_device *bus,
                                       const char               *name)
{
    struct rt_device *device;
    RT_ASSERT(bus != RT_NULL);

    device = &bus->parent;

    device->user_data = bus;

    /* set device type */
    device->type    = RT_Device_Class_I2CBUS;
    /* initialize device interface */
    device->init    = i2c_bus_device_init;
    device->open    = i2c_bus_device_open;
    device->close   = i2c_bus_device_close;
    device->read    = i2c_bus_device_read;
    device->write   = i2c_bus_device_write;
    device->control = i2c_bus_device_control;

    /* register to device manager */
    rt_device_register(device, name, RT_DEVICE_FLAG_RDWR| RT_DEVICE_FLAG_INT_TX | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_DMA_RX);

    return RT_EOK;
}
