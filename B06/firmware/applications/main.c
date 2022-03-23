/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-06     SummerGift   first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#define bat_led 	GET_PIN(C,0)
#define wifi_led 	GET_PIN(C,1)
#define uwb_on		GET_PIN(A,5)
#define wifi_on		GET_PIN(C,3)



static void bat_led_status(rt_uint8_t sta){
	if(sta) rt_pin_write(bat_led,PIN_HIGH);
	else    rt_pin_write(bat_led,PIN_LOW);
}

static void wifi_led_status(rt_uint8_t sta){
	if(sta) rt_pin_write(wifi_led,PIN_HIGH);
	else    rt_pin_write(wifi_led,PIN_LOW);
}

static void uwb_on_status(rt_uint8_t sta){
	if(sta) rt_pin_write(uwb_on,PIN_HIGH);
	else    rt_pin_write(uwb_on,PIN_LOW);
}

static void wifi_on_status(rt_uint8_t sta){
	if(sta) rt_pin_write(wifi_on,PIN_HIGH);
	else    rt_pin_write(wifi_on,PIN_LOW);
}

static void pin_init(void){
	rt_pin_mode(bat_led,PIN_MODE_OUTPUT);
	rt_pin_mode(wifi_led,PIN_MODE_OUTPUT);
	rt_pin_mode(uwb_on,PIN_MODE_OUTPUT);
	rt_pin_mode(wifi_on,PIN_MODE_OUTPUT);
	uwb_on_status(1);
	wifi_on_status(1);
}
int main(void)
{
   rt_uint16_t count = 1;
    pin_init();
		
    while (count++)
    {
      
			rt_thread_delay(1000);
    }
}
