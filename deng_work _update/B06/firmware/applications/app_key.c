#include "driver_key.h"
#include "app_key.h"
#include "driver_led.h"
#include "stdbool.h"
#include "app_battery.h"
#include "param.h"


rt_uint8_t wifi_dev_sta = 0;
rt_uint8_t connect_indicator_flag = 0;
static bool key_running = true;
extern rt_event_t bat_event;//电池事件
rt_uint8_t led_change_num = 0x00;//不同状态下的led灯的变化:0:正常状态 ，1:升级状态 2:被APP选中状态
rt_uint16_t Base_num = 0xffff;
bool uwb_running = false;
bool wifi_running = false;
extern rt_uint8_t update_led;


void app_key_entry(void *arg){
 	key_running = true;
	static bool module_power_open = false;
	static rt_uint8_t power_led_flag = 0;
	static rt_uint8_t led_count = 0;
	static rt_uint8_t indicator_led_count = 0;
	static bool have_wifi_reset_flag = false;
	static bool have_open_power = false;
	static rt_uint8_t reset_time_count = 0;
	rt_uint8_t status_change = 0;
	rt_uint8_t ind_status_change = 0;
	struct save_param s_param;
	rt_uint8_t base_status_change = 0;
	//初始化电源按键，电源电压输出
	battery_led_init();
	indicator_led_init();
	uwb_power_out_init();
	wifi_power_out_init();
	while(bat_event == RT_NULL){
		rt_thread_delay(10);
	}
	rt_kprintf("\r\n[KEY_POWER] key and power out init successed\r\n");
	while(1){
		while(key_running){
			//电池电量为零
			if(rt_event_recv(bat_event,BAT_CAPACITY_IS_ZERO,RT_EVENT_FLAG_AND|RT_EVENT_FLAG_CLEAR,1,RT_NULL) == RT_EOK){
				power_led_flag = 0x01;
				if(led_change_num == 0x01 && module_power_open == true) module_power_open = true;
				else                                                  	module_power_open = false;
			}
			//电池电量低
			if(rt_event_recv(bat_event,BAT_CAPACITY_IS_LOW,RT_EVENT_FLAG_AND|RT_EVENT_FLAG_CLEAR,1,RT_NULL) == RT_EOK){
				power_led_flag = 0x02;
				if(have_open_power == false){
					module_power_open = true;
				}
			}
			//电池正常
			if(rt_event_recv(bat_event,BAT_CAPACITY_IS_USED,RT_EVENT_FLAG_AND|RT_EVENT_FLAG_CLEAR,1,RT_NULL) == RT_EOK){
				power_led_flag = 0x03;
				if(have_open_power == false){
					module_power_open = true;
				}
			}
			if(module_power_open == true){
				uwb_running = true;
				uwb_power_out_enable();
						
				if(Base_num == 0x0000 && wifi_dev_sta){
					wifi_running = true;
					wifi_power_out_enable();
				}
				else if(Base_num != 0x0000 && wifi_dev_sta){
					wifi_running = false;
					wifi_power_out_disable();
					connect_indicator_flag = 0x00;
				}
				if(wifi_dev_sta == 0){
					wifi_running = true;
					wifi_power_out_enable();
				}
        if(led_change_num == 0x00){
					//电源指示灯
          if(power_led_flag == 0x01){
                led_count = 0;
                battery_led_disable();
           }else if(power_led_flag  == 0x02){
                if(led_count ++ >= 15){
                 led_count = 0;
                 status_change = !status_change;
            if(status_change) battery_led_disable();
						else              battery_led_enable();
          }
        }
				else if(power_led_flag == 0x03){
					led_count = 0;
					battery_led_enable();
				}
								
				//wifi数据指示灯
				if(connect_indicator_flag == 0x00){
					indicator_led_count = 0;
					indicator_led_disable();
				}else if(connect_indicator_flag == 0x01){
					if(indicator_led_count ++ >= 10){
						indicator_led_count = 0;
						ind_status_change = !ind_status_change;
						if(ind_status_change)indicator_led_disable();
						else 				 indicator_led_enable();
					}
				}else if(connect_indicator_flag == 0x02){
					if(indicator_led_count ++ >= 80){
						indicator_led_count = 0;
						ind_status_change = !ind_status_change;
						if(ind_status_change)indicator_led_disable();
						else 				 indicator_led_enable();
					}
				}else if(connect_indicator_flag == 0x03){
					indicator_led_count = 0;
					indicator_led_enable();
				}
							
     }
     else if(led_change_num == 0x01){
        if(update_led){
					indicator_led_enable();
					battery_led_enable();
				}
				else {
					indicator_led_disable();
					battery_led_disable();
				}
     }
     else if(led_change_num == 0x02){
				if(led_count ++ >= 30){
					led_count = 0;
					base_status_change = !base_status_change;
					if(base_status_change){
						indicator_led_disable();
						battery_led_disable();
					}else{
						indicator_led_enable();
						battery_led_enable();
					}
				}
                
      }
		}
		else{
			uwb_running 	= false;
			wifi_running 	= false;
			indicator_led_disable();
			battery_led_disable();
			uwb_power_out_disable();
			wifi_power_out_disable();
		}
				//rt_kprintf("wifi_dev_sta = %d\r\n",wifi_dev_sta);
		rt_thread_delay(10);
	   }
	rt_thread_delay(100);
	}
}













