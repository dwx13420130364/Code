/*
 * File      : hwtimer.c
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
 * Date           Author         Notes
 * 2015-08-31     heyuanjie87    first version
 */

#include <rtthread.h>
#include <rtdevice.h>

rt_inline rt_uint32_t timeout_calc(rt_hwtimer_t *timer, rt_hwtimerval_t *tv)
{
    float overflow;
    float timeout;
    rt_uint32_t counter;
    int i, index;
    float tv_sec;
    float devi_min = 1;
    float devi;

    /* 把定时器溢出时间和定时时间换算成秒 */
    overflow = timer->info->maxcnt/(float)timer->freq;
    tv_sec = tv->sec + tv->usec/(float)1000000;

    if (tv_sec < (1/(float)timer->freq))
    {
        /* 定时时间小于计数周期 */
        i = 0;
        timeout = 1/(float)timer->freq;
    }
    else
    {
        for (i = 1; i > 0; i ++)
        {
            timeout = tv_sec/i;

            if (timeout <= overflow)
            {
                counter = timeout*timer->freq;
                devi = tv_sec - (counter/(float)timer->freq)*i;
                /* 计算最小误差 */
                if (devi > devi_min)
                {
                    i = index;
                    timeout = tv_sec/i;
                    break;
                }
                else if (devi == 0)
                {
                    break;
                }
                else if (devi < devi_min)
                {
                    devi_min = devi;
                    index = i;
                }
            }
        }
    }

    timer->cycles = i;
    timer->reload = i;
    timer->period_sec = timeout;
    counter = timeout*timer->freq;

    return counter;
}

static rt_err_t rt_hwtimer_init(struct rt_device *dev)
{
    rt_err_t result = RT_EOK;
    rt_hwtimer_t *timer;

    timer = (rt_hwtimer_t *)dev;
    /* 尝试将默认计数频率设为1Mhz */
    if ((1000000 <= timer->info->maxfreq) && (1000000 >= timer->info->minfreq))
    {
        timer->freq = 1000000;
    }
    else
    {
        timer->freq = timer->info->minfreq;
    }
    timer->mode = HWTIMER_MODE_ONESHOT;
    timer->cycles = 0;
    timer->overflow = 0;

    if (timer->ops->init)
    {
        timer->ops->init(timer, 1);
    }
    else
    {
        result = -RT_ENOSYS;
    }

    return result;
}

static rt_err_t rt_hwtimer_open(struct rt_device *dev, rt_uint16_t oflag)
{
    rt_err_t result = RT_EOK;
    rt_hwtimer_t *timer;

    timer = (rt_hwtimer_t *)dev;
    if (timer->ops->control != RT_NULL)
    {
        timer->ops->control(timer, HWTIMER_CTRL_FREQ_SET, &timer->freq);
    }
    else
    {
        result = -RT_ENOSYS;
    }

    return result;
}

static rt_err_t rt_hwtimer_close(struct rt_device *dev)
{
    rt_err_t result = RT_EOK;
    rt_hwtimer_t *timer;

    timer = (rt_hwtimer_t*)dev;
    if (timer->ops->init != RT_NULL)
    {
        timer->ops->init(timer, 0);
    }
    else
    {
        result = -RT_ENOSYS;
    }

    dev->flag &= ~RT_DEVICE_FLAG_ACTIVATED;
    dev->rx_indicate = RT_NULL;

    return result;
}

static rt_size_t rt_hwtimer_read(struct rt_device *dev, rt_off_t pos, void *buffer, rt_size_t size,...)
{
    rt_hwtimer_t *timer;
    rt_hwtimerval_t tv;
    rt_uint32_t cnt;
    float t;

    timer = (rt_hwtimer_t *)dev;
    if (timer->ops->count_get == RT_NULL)
        return 0;

    cnt = timer->ops->count_get(timer);
    if (timer->info->cntmode == HWTIMER_CNTMODE_DW)
    {
        cnt = timer->info->maxcnt - cnt;
    }

    t = timer->overflow * timer->period_sec + cnt/(float)timer->freq;
    tv.sec = t;
    tv.usec = (t - tv.sec) * 1000000;
    size = size > sizeof(tv)? sizeof(tv) : size;
    rt_memcpy(buffer, &tv, size);

    return size;
}

static rt_size_t rt_hwtimer_write(struct rt_device *dev, rt_off_t pos, const void *buffer, rt_size_t size,...)
{
    rt_uint32_t t;
    rt_hwtimer_mode_t opm = HWTIMER_MODE_PERIOD;
    rt_hwtimer_t *timer;

    timer = (rt_hwtimer_t *)dev;
    if ((timer->ops->start == RT_NULL) || (timer->ops->stop == RT_NULL))
        return 0;

    if (size != sizeof(rt_hwtimerval_t))
        return 0;

    if ((timer->cycles <= 1) && (timer->mode == HWTIMER_MODE_ONESHOT))
    {
        opm = HWTIMER_MODE_ONESHOT;
    }
    timer->ops->stop(timer);
    timer->overflow = 0;

    t = timeout_calc(timer, (rt_hwtimerval_t*)buffer);
    if (timer->ops->start(timer, t, opm) != RT_EOK)
        size = 0;

    return size;
}

static rt_err_t rt_hwtimer_control(struct rt_device *dev, rt_uint8_t cmd, void *args)
{
    rt_err_t result = RT_EOK;
    rt_hwtimer_t *timer;

    timer = (rt_hwtimer_t *)dev;

    switch (cmd)
    {
    case HWTIMER_CTRL_STOP:
    {
        if (timer->ops->stop != RT_NULL)
        {
            timer->ops->stop(timer);
        }
        else
        {
            result = -RT_ENOSYS;
        }
    }
    break;
    case HWTIMER_CTRL_FREQ_SET:
    {
        rt_uint32_t *f;

        if (args == RT_NULL)
        {
            result = -RT_EEMPTY;
            break;
        }

        f = (rt_uint32_t*)args;
        if ((*f > timer->info->maxfreq) || (*f < timer->info->minfreq))
        {
            result = -RT_ERROR;
            break;
        }

        if (timer->ops->control != RT_NULL)
        {
            result = timer->ops->control(timer, cmd, args);
            if (result == RT_EOK)
            {
                timer->freq = *f;
            }
        }
        else
        {
            result = -RT_ENOSYS;
        }
    }
    break;
    case HWTIMER_CTRL_INFO_GET:
    {
        if (args == RT_NULL)
        {
            result = -RT_EEMPTY;
            break;
        }

        *((struct rt_hwtimer_info*)args) = *timer->info;
    }
    case HWTIMER_CTRL_MODE_SET:
    {
        rt_hwtimer_mode_t *m;

        if (args == RT_NULL)
        {
            result = -RT_EEMPTY;
            break;
        }

        m = (rt_hwtimer_mode_t*)args;

        if ((*m != HWTIMER_MODE_ONESHOT) && (*m != HWTIMER_MODE_PERIOD))
        {
            result = -RT_ERROR;
            break;
        }

        timer->mode = *m;
    }
    break;
    default:
    {
        result = -RT_ENOSYS;
    }
    break;
    }

    return result;
}

void rt_device_hwtimer_isr(rt_hwtimer_t *timer)
{
    RT_ASSERT(timer != RT_NULL);

    timer->overflow ++;

    if (timer->cycles != 0)
    {
        timer->cycles --;
    }

    if (timer->cycles == 0)
    {
        timer->cycles = timer->reload;

        if (timer->mode == HWTIMER_MODE_ONESHOT)
        {
            if (timer->ops->stop != RT_NULL)
            {
                timer->ops->stop(timer);
            }
        }

        if (timer->parent.rx_indicate != RT_NULL)
        {
            timer->parent.rx_indicate(&timer->parent, sizeof(struct rt_hwtimerval));
        }
    }
}

rt_err_t rt_device_hwtimer_register(rt_hwtimer_t *timer, const char *name, void *user_data)
{
    struct rt_device *device;

    RT_ASSERT(timer != RT_NULL);
    RT_ASSERT(timer->ops != RT_NULL);
    RT_ASSERT(timer->info != RT_NULL);

    device = &(timer->parent);

    device->type        = RT_Device_Class_Timer;
    device->rx_indicate = RT_NULL;
    device->tx_complete = RT_NULL;

    device->init        = rt_hwtimer_init;
    device->open        = rt_hwtimer_open;
    device->close       = rt_hwtimer_close;
    device->read        = rt_hwtimer_read;
    device->write       = rt_hwtimer_write;
    device->control     = rt_hwtimer_control;
    device->user_data   = user_data;

    return rt_device_register(device, name, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_STANDALONE);
}
