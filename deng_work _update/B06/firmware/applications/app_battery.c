#include "app_battery.h"
#include "driver_battery.h"
#include "stdbool.h"

rt_event_t bat_event = RT_NULL;//低电量
static bool battery_running = true;
#define FILTER_NUM  100
#define RAW_NUM     40
static rt_uint16_t filter_buf[FILTER_NUM];
static rt_uint32_t filter_index = 0;

#define BATTERY_PERCENT_LUT_SIZE 21
static const struct{
	float voltage;
	float pervent;

}my_bat_lut[] = {
	{6.04f,0.0f},
	{6.70f, 5.0f},
  {6.8399f, 10.0f},
  {6.9909f, 15.0f},
  {7.0178f, 20.0f},
  {7.0486f, 25.0f},
  {7.0769f, 30.0f},
  {7.1164f, 35.0f},
  {7.1522f, 40.0f},
  {7.1957f, 45.0f},
  {7.2565f, 50.0f},
  {7.2973f, 55.0f},
  {7.3680f, 60.0f},
  {7.4313f, 65.0f},
  {7.4902f, 70.0f},
  {7.5384f, 75.0f},
  {7.5941f, 80.0f},
  {7.6397f, 85.0f},
  {7.6729f, 90.0f},
  {7.7220f, 95.0f},
  {7.8408f, 100.0f}
};


float my_compute_battery_percenttage(float vbat){
	vbat += 4.1f;
	float percent = 0.1f;
	int i ;
	if(vbat <= my_bat_lut[0].voltage){
		percent = 0.0f;
	}else if(vbat >= my_bat_lut[BATTERY_PERCENT_LUT_SIZE-1].voltage){
		percent = 100.0f;
	}else{
		i = 0;
		while(vbat >= my_bat_lut[i].voltage)i++;
		percent += my_bat_lut[i-1].pervent+\
		(vbat-my_bat_lut[i-1].voltage)*\
		(my_bat_lut[i].pervent - my_bat_lut[i-1].pervent)/\
		(my_bat_lut[i].voltage - my_bat_lut[i-1].voltage);
	}

	return percent;
}

/*************************************************************/
#define BATTERY_FLY_TAB_SIZE 13
static const struct{
	float voltage;
	float pervent;
}my_bat_fly_tab[] = {
	{2.800f,0.0f},
	{3.253f,2.0f},
	{3.450f,5.0f},
	{3.524f,10.0f},
	{3.566f,20.0f},
	{3.607f,30.0f},
	{3.663f,40.0f},
	{3.727f,50.0f},
	{3.807f,60.0f},
	{3.894f,70.0f},
	{3.987f,80.0f},
	{4.082f,90.0f},
	{4.122f,100.0f},
};


float _compute_battery_percenttage(float vbat){

	float percent = 0.1f;
	int i ;
	if(vbat <= my_bat_fly_tab[0].voltage){
		percent = 0.0f;
	}else if(vbat >= my_bat_fly_tab[BATTERY_FLY_TAB_SIZE-1].voltage){
		percent = 100.0f;
	}else{
		i = 0;
		while(vbat >= my_bat_fly_tab[i].voltage)i++;
		percent += my_bat_fly_tab[i-1].pervent+\
		(vbat-my_bat_fly_tab[i-1].voltage)*\
		(my_bat_fly_tab[i].pervent - my_bat_fly_tab[i-1].pervent)/\
		(my_bat_fly_tab[i].voltage - my_bat_fly_tab[i-1].voltage);
	}

	return percent;
}









rt_uint16_t bat_adc_swap_filter(uint16_t in_data)
{
	rt_uint32_t temp_sum = 0;
	rt_uint32_t i = 0;
	static rt_uint32_t last_recv_data = 0;
	//第一次进入，滤波数据初始化为第一次的数据
	static bool first_enter = true;
	if(first_enter)
	{
		first_enter = false;
		for(i = 0; i < FILTER_NUM; i++)
		{
			filter_buf[i] = in_data;
		}
	}
	else
	{
		//突变数据
		if(in_data <=(last_recv_data/50))
		{
			for(i = 0; i < FILTER_NUM; i++)
			{
				filter_buf[i] = in_data;
			}
		}
		else if(in_data >=(last_recv_data*50))
		{
			for(i = 0; i < FILTER_NUM; i++)
			{
				filter_buf[i] = in_data;
			}
		}
		else
		{
			filter_buf[filter_index++] = in_data;
			if(filter_index == FILTER_NUM)
			{
				filter_index = 0;
			}
		}
	}
	for(i = 0; i < FILTER_NUM; i++)
	{
		temp_sum += filter_buf[i];
	}
	last_recv_data = in_data;
	return temp_sum/FILTER_NUM;
}

//获取平均值，去掉del_len头尾

rt_uint16_t bat_adc_get_average(uint16_t *in_data,uint8_t len,uint8_t del_len)
{
	rt_uint32_t temp_val=0;
	rt_uint8_t  i = 0;
	if(len > del_len)
	{
		for(i = del_len; i < len-del_len; i++)
		{
			temp_val += in_data[i];
		}
		temp_val = temp_val/(len - 2*del_len);
	}
	return temp_val;
} 


//n为数组a的元素个数

void bubble_sort(uint16_t *data,uint8_t n)
{
	rt_uint8_t i = 0;
	rt_uint8_t j = 0;
	rt_uint16_t temp = 0;
	if(data != RT_NULL)
	{
		//一定进行N-1轮比较
		for(i=0; i<n-1; i++)
		{
			//每一轮比较前n-1-i个，即已排序好的最后i个不用比较
			for(j=0; j<n-1-i; j++)
			{
				if(data[j] > data[j+1])
				{
					temp = data[j];
					data[j] = data[j+1];
					data[j+1]=temp;
				}
			}
		} 
	}
}

bool bat_health = false;
rt_uint8_t global_percent = 0;
static rt_uint16_t bat_adc_value[RAW_NUM] = {0};
extern rt_uint16_t Base_num;
void app_bat_entry(void* arg){
	battery_running = true;
	rt_uint16_t temp_adc_value = 0;
	rt_uint16_t temp_value_avr = 0;
	rt_uint16_t temp_swap_value = 0;
	float bat_voltage = 0;
	rt_uint16_t bat_value_index = 0;
	bat_adc_init();
	while(bat_event == RT_NULL)
	{
		bat_event = rt_event_create("bat_event",RT_IPC_FLAG_FIFO);
		if(bat_event == RT_NULL)
		{
			rt_kprintf("[BAT] low battery sem creat falied\r\n");
		}
	}
	rt_kprintf("\r\n[BAT] bat thread init successed\r\n");
	while(1){
		while(battery_running){
			while(bat_value_index < RAW_NUM)
			{
				//获取ADC的值
				temp_adc_value = 0;
				temp_adc_value = bat_get_adc_value();
				//将获得数据存入数组中进行排序
				bat_adc_value[bat_value_index++] = temp_adc_value;
			}
			if(bat_value_index == RAW_NUM){
				bat_value_index = 0;
				//缓冲数据进行排序                
				temp_value_avr = 0;
				temp_swap_value =0; 
				bubble_sort(bat_adc_value,RAW_NUM);
				//取缓冲数据的平均值
				temp_value_avr = bat_adc_get_average(bat_adc_value,RAW_NUM,2);
				//进行平滑滤波
				temp_swap_value = bat_adc_swap_filter(temp_value_avr);
				//转化为电池电压
				bat_voltage = ((float)(temp_swap_value*3.24f/4095))*3;
				//global_percent = (uint8_t)(my_compute_battery_percenttage(bat_voltage-0.4f));
				//rt_kprintf("global_percent = %d,bat_voltage = %0.4f\r\n",global_percent,bat_voltage);
//				if(Base_num == 0x0000){
//					global_percent = (rt_uint8_t)(global_percent*4/5);
//				}
				global_percent = _compute_battery_percenttage(bat_voltage - 0.18f);
				if(global_percent < 5)	
				{
					rt_event_send(bat_event,BAT_CAPACITY_IS_ZERO);
				}
				else if(global_percent <= 20)
				{
					rt_event_send(bat_event,BAT_CAPACITY_IS_LOW);
				}
				else
				{
					rt_event_send(bat_event,BAT_CAPACITY_IS_USED);
				}
				bat_health = true;
				rt_memset(bat_adc_value, 0, sizeof(bat_adc_value));
				bat_value_index = 0;
			}
			rt_thread_delay(200);
		}
		rt_thread_delay(20);
	}
}


