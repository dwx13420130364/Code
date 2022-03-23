#include "app_uwb.h"
#include "stdbool.h"


extern bool uwb_running;

//获取UWB的编号
static rt_uint8_t get_num_cmd[14];//
void get_base_num(rt_device_t dev){
	rt_uint8_t checksum = 0,index = 0;
	rt_memset(get_num_cmd,0,sizeof(get_num_cmd));
	get_num_cmd[index++] = 0xaa;
	get_num_cmd[index++] = 0xad;
	get_num_cmd[index++] = sizeof(get_num_cmd);
	while(index<13&&index>2)get_num_cmd[index++] = 0x00;
	for(rt_uint8_t i = 0;i< index;i++){
		checksum += get_num_cmd[i];
	}
	get_num_cmd[index++] = checksum;
	
	if(rt_device_write(dev,0,get_num_cmd,index) != index){
		 rt_kprintf("[UWB] send base num read cmd failed\r\n");
	}
}


//发送基站的电量和版本号给UWB
extern rt_uint8_t wifi_dev_sta;//表示此基站是否有WiFi模块
extern rt_uint8_t global_percent;//基站电量百分值
static rt_uint8_t send_tick_cmd[11];//发送基站的心跳信息
void Send_Base_Data_Read_Cmd(rt_device_t dev){
	rt_uint8_t checksum = 0;
	rt_uint8_t index = 0;
	rt_uint8_t percent = 0;
	rt_uint8_t wifi_module = 0x00;
	rt_memset(send_tick_cmd,0,sizeof(send_tick_cmd));
	if(global_percent < percent){
       global_percent = percent;
   }else global_percent = global_percent;
   percent = global_percent;
	 if(wifi_dev_sta == 0x00)wifi_module = 0x00;
	 if(wifi_dev_sta == 0x01)wifi_module = 0xaa;
	 if(wifi_dev_sta == 0x02)wifi_module = 0x55;
	 
	 send_tick_cmd[index++] = 0xab;
	 send_tick_cmd[index++] = 0x00;
	 send_tick_cmd[index++] = sizeof(send_tick_cmd);
	 send_tick_cmd[index++] = HG_HARDVERSION;
	 send_tick_cmd[index++] = BOOT_VERSION*10+BOOT_SUBVERSION;
	 send_tick_cmd[index++] =	BOOT_REVISION*10+BOOT_BETA;
	 send_tick_cmd[index++] =	HG_VERSION*10+HG_SUBVERSION;
	 send_tick_cmd[index++] =	HG_REVISION*10+HG_BETA;
	 send_tick_cmd[index++] =	global_percent;
	 send_tick_cmd[index++] =	wifi_module;
	 for(rt_uint8_t i = 0;i<index;i++){
			checksum += send_tick_cmd[i];
	 }
	 send_tick_cmd[index++] = checksum;
	 if(rt_device_write(dev, 0, send_tick_cmd, index) !=  index){
       rt_kprintf("[UWB] send base data read cmd failed\r\n");
   }	 
}

#define UWB_BUFFER_NUM 12
extern bool get_ap_trans_flag(void);
static rt_uint8_t uwb_trans_buf[UWB_BUFFER_NUM][1024] = {0};
static rt_uint8_t choice = 0;
extern rt_mq_t firmware_update;//升级消息队列
bool get_uwb_num_sta = false;
extern rt_uint16_t Base_num;//此基站的编号
extern rt_uint8_t led_change_num;//此基站被APP选中
bool wifi_tick_flag = false; //发送WiFi心跳允许位，防止黏包
rt_uint8_t uwb_update_flag = 0;//中继器升级标志位
extern bool wifi_running;
extern rt_uint8_t update_led ;
extern bool Update_running;



extern bool ap_init_flag;
extern void set_ap_trans_falg(bool flag);
void app_uwb_entry(void *arg){
	rt_uint16_t buf_len = 0;
	rt_uint16_t read_cnt = 0;
	__IO rt_uint16_t data_len = 0;
	rt_uint16_t tick_count = 0;
	rt_uint16_t senf_wifi_cnt = 0;
	rt_uint16_t count = 0;
	rt_device_t uwb_trans_dev = RT_NULL;
	rt_device_t wifi_trans_dev = RT_NULL;
	uwb_trans_dev = rt_device_find("uart1");
  if(uwb_trans_dev == RT_NULL)rt_kprintf("[uwb] uart1 device not found\r\n");
  else  rt_device_open(uwb_trans_dev,RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_DMA_RX | RT_DEVICE_FLAG_DMA_TX);
	wifi_trans_dev = rt_device_find("uart6");
  if(wifi_trans_dev == RT_NULL)rt_kprintf("[wifi] uart6 device not found\r\n");
  else  rt_device_open(wifi_trans_dev,RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_DMA_RX | RT_DEVICE_FLAG_DMA_TX);
	rt_thread_delay(1000);
  rt_kprintf("\r\n[uwb] get data thread running\r\n");
  buf_len = sizeof(uwb_trans_buf[0]);
  rt_memset(uwb_trans_buf[choice], 0, buf_len);
	while(1){
		while(uwb_running){
			if(get_uwb_num_sta == false){
				if(count++>10){
					count = 0;
					get_base_num(uwb_trans_dev);
				 }
			 read_cnt = rt_device_read(uwb_trans_dev, 0 , uwb_trans_buf[choice]+data_len, buf_len-data_len);
			if(read_cnt){
				data_len += read_cnt;
			}
			else{
				if(data_len > 0){
					 if(uwb_trans_buf[choice][0] == 0xaa && uwb_trans_buf[choice][1] == 0xad ){
						Base_num = uwb_trans_buf[choice][6]<<8|uwb_trans_buf[choice][5];
						get_uwb_num_sta = true;
					}else{
							get_uwb_num_sta = false;
					 }
					// rt_device_write(wifi_trans_dev, 0, uwb_trans_buf[choice], data_len);
					 rt_kprintf("Base_num = %d\r\n",Base_num);
					 rt_memset(uwb_trans_buf[choice], 0, buf_len);
						data_len = 0;
						choice = 0;
						count = 0;
					}
				}
			}
			else if(get_uwb_num_sta == true){
				count = 0;
				if(tick_count ++ >= 3000){//一秒发送一次心跳
					tick_count = 0;
					if(uwb_update_flag == 0){//如果是处于升级状态则停止发送心跳
						Send_Base_Data_Read_Cmd(uwb_trans_dev);
					}
				}
				read_cnt = rt_device_read(uwb_trans_dev,0,uwb_trans_buf[choice]+data_len,buf_len-data_len);
				if(read_cnt){
					data_len += read_cnt;
				}else {
					if(data_len > 0){
						senf_wifi_cnt = 0;
						if(uwb_trans_buf[choice][0] == 0xCC && (uwb_trans_buf[choice][1] == 0x02 || uwb_trans_buf[choice][1] == 0x03)){
							rt_mq_send(firmware_update,uwb_trans_buf[choice],data_len);
						}
						else if(uwb_trans_buf[choice][0] == 0xCC && uwb_trans_buf[choice][1] == 0x01 ){
								uwb_update_flag = 1;
								wifi_running = false;
								ap_init_flag = true;
								led_change_num = 0x01;
								update_led = 1;
								//set_ap_trans_falg(false);
						}
				
						else {
							if(uwb_trans_buf[choice][0] == 0XCC && uwb_trans_buf[choice][1] == 0xFE){
								if(uwb_trans_buf[choice][5] == 0x01){
									led_change_num = 0x02;
								}
								else led_change_num = 0x00;
							}
							if(get_ap_trans_flag()){
									rt_device_write(wifi_trans_dev,0,uwb_trans_buf[choice],data_len);
							}
						}
						choice++;
						if(choice > (UWB_BUFFER_NUM - 1)){
							choice = 0;
						}
						data_len = 0;
						rt_memset(uwb_trans_buf[choice], 0, buf_len);
					}
					else{
						if(senf_wifi_cnt < 8){
							senf_wifi_cnt++;
							wifi_tick_flag = false;
						}else {
							wifi_tick_flag = true;
							senf_wifi_cnt = 0;
						}
					}
				}
			}
			rt_thread_delay(1);
		}
		rt_thread_delay(1);
	}
}

