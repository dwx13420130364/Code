#include "app_wifi_recv.h"
#include "param.h"
#include "app_uwb.h"
#include "stmflash.h"


static rt_uint8_t buffer[1024];
static rt_uint16_t parse_total_len = 0;
static rt_uint16_t decode_state = 0;
extern bool wifi_running;
void get_parse_buffer(rt_uint8_t *copy_buffer,rt_uint16_t len){
	rt_memcpy(copy_buffer, buffer, len);
	rt_memset(buffer, 0, sizeof(buffer));
}

void clear_parse_buffer(void)
{
	rt_memset(buffer, 0, sizeof(buffer));
}

void get_parse_total_len(rt_uint16_t *data_len)
{
	*data_len = parse_total_len;
	parse_total_len = 0;
}

void clear_parse_total_len(void)
{
	parse_total_len = 0;
}

rt_uint8_t wifi_uwb_datasta = 0;
rt_uint8_t update_uwblock = 0;
rt_uint8_t lock_uwbtick = 0;
//对字符串进行解析储存，发送数据
int inf_wifi_parse_char(uint8_t ch){
	__IO  rt_uint16_t total_len = 0;
	rt_uint8_t recv_successed_flag = 0;
	if(decode_state == 0){
		buffer[decode_state] = ch;
		if((ch == 0x54) || (ch == 0xFE) || (ch == 0x68) ||(ch == 0x52)||(ch == 0XAA)||(ch == '<')||(ch == 0xAF))
        {
			if(ch == 0XAA)wifi_uwb_datasta = 1;
			else wifi_uwb_datasta = 0;
			if(ch == '<')update_uwblock = 1;
			else update_uwblock = 0;
			if(ch == 0xFE)lock_uwbtick = 1;
			else lock_uwbtick = 0;
            decode_state++;
        }else decode_state = 0;
	}
	
	else{
		switch(buffer[0]){
			//基站设置
			case 0x68:
				buffer[decode_state++] = ch;
				if((buffer[1] > 50) || (buffer[1] <= 0)){
					decode_state = 0;
				}
				if(decode_state == buffer[1]){
					if(buffer[decode_state-1] != 0x16){
						parse_total_len = 0;
						decode_state = 0;
					}
					else{
						parse_total_len = decode_state;
						decode_state = 0;
						recv_successed_flag = 0x02;
					}
				}
                else if(decode_state > buffer[1]){
                    decode_state = 0;
                }
			break;
			//mavlink
			case 0xFE:
				buffer[decode_state++] = ch;
				if(decode_state == (buffer[1]+8)){
					parse_total_len = decode_state;
					decode_state = 0;
					recv_successed_flag = 0x01;
				}
                else if(decode_state > (buffer[1]+8)){
                    decode_state = 0;
                }
			break;
			case 0x52:
				buffer[decode_state++] = ch;
				if(buffer[1] == 0x00){
					if(decode_state == 32){
						parse_total_len = decode_state;
						decode_state = 0;
						recv_successed_flag = 0x01;
					}
				}
				else if(buffer[1] == 0x01){
					if(decode_state == 984){
						parse_total_len = decode_state;
						decode_state = 0;
						recv_successed_flag = 0x01;
					}
				}
				else{
					decode_state = 0;
				}
			break;
			//uwb数据
			case 0x54:
				buffer[decode_state++] = ch;
				if((buffer[1] == 0xf0) || (buffer[1] == 0x00)){
					if(decode_state == 128){
						parse_total_len = decode_state;
						decode_state = 0;
						recv_successed_flag = 0x01;
					}
				}
				else if(buffer[1] == 0xf1){
					if(decode_state > 10){
						total_len = buffer[8] + (buffer[9]*256);
						if(total_len > 0){
 							if(decode_state == (total_len + 11)){
								parse_total_len = decode_state;
								decode_state = 0;
								recv_successed_flag = 0x01;
							}
                            else if(decode_state > (total_len + 11)){
                                decode_state = 0;
                            }
						}
						else{
							decode_state = 0;
						}
					}
				}
				else{
					decode_state = 0;
				}
             break;
			case 0XAA:
				buffer[decode_state++] = ch;
				if(decode_state==buffer[2]){
					if(buffer[3] == 0) {
						recv_successed_flag = 0x01;
					}
					parse_total_len = decode_state;
					decode_state = 0;	
				}
			break;
			case '<':
				buffer[decode_state++] = ch;
				if(buffer[1]=='O'&&buffer[2]=='T'&&buffer[3]=='A'){
					if(buffer[6]=='U'){
						if(decode_state == 64){
							recv_successed_flag = 0x01;
							parse_total_len = decode_state;
							decode_state = 0;
						}
					}
					if(buffer[6]=='d'){
						if(decode_state == (buffer[52]*256+buffer[51]+57)){
							recv_successed_flag = 0x01;
							parse_total_len = decode_state;
							decode_state = 0;
						}	
					}
				}
			break;
			case 0xAF:
				buffer[decode_state++] = ch;
				total_len =  *(uint16_t*)&buffer[2];
				if(decode_state == total_len){
					recv_successed_flag = 0x01;
					parse_total_len = decode_state;
					decode_state = 0;
				}
			break;
			default:
				decode_state = 0;
			break;
        }
    }
	if(decode_state > sizeof(buffer)){
		decode_state = 0;
	}
	return recv_successed_flag;
}

int generate_ask(uint8_t *data, uint8_t flag, uint8_t status)
{
	uint8_t data_index = 0;
	uint8_t checksum = 0;
	if(data == NULL)
	{
		rt_kprintf("[5.8G_wifi] generate ask frame error\r\n");
		return -RT_ERROR;
	}
	data[data_index++] = 0x68;
	data[data_index++] = 0x00;
	data[data_index++] = flag;
	checksum += flag;
	if(status == 0x01)
	{
		data[data_index++] = 'O';
		checksum += 'O';
		data[data_index++] = 'K';
		checksum += 'K';
	}
	//心跳数据
	else if(status == 0x02)
	{
		data[data_index++] = 'T';
		checksum += 'T';
		data[data_index++] = 'K';
		checksum += 'K';
		data[data_index++] = HG_VERSION;
		checksum += HG_VERSION;
		data[data_index++] = HG_SUBVERSION;
		checksum += HG_SUBVERSION;
		data[data_index++] = HG_REVISION;
		checksum += HG_REVISION;
		data[data_index++] = HG_BETA;
		checksum += HG_BETA;
	}
	else
	{
		data[data_index++] = 'E';
		checksum += 'E';
		data[data_index++] = 'E';
		checksum += 'E';
	}
	data[data_index++] = checksum;
	data[data_index++] = 0x16;
	data[1] = data_index;
	return data_index;
}
void rt_reboot(void)
{
    NVIC_SystemReset();
}
//对字符串进行解析存储，发送数据
int wifi_set_storage(uint8_t *in_data,rt_device_t dev)
{
	struct save_param r_param;
	struct save_param w_param;
	uint8_t checksum = 0;
	uint8_t ssid_len = 0;
	uint8_t password_len = 0;
	uint8_t *password_point = RT_NULL;
	uint8_t ask_data[16] = {0};
	//检查帧头
    if(in_data[0] != 0x68)
	{
		rt_kprintf("[5.8G_wifi] wifi set frame head error\r\n");
		return -RT_ERROR;
	}
	//检查帧尾
	if(in_data[in_data[1]-1] != 0x16)
	{
		rt_kprintf("[5.8G_wifi] wifi set frame tail error\r\n");
		return -RT_ERROR;
	}
	for(uint8_t i = 2; i < (in_data[1]- 2); i++)
	{
		checksum += in_data[i];
	}
	//检查校验和
	if(checksum != in_data[in_data[1]-2])
	{
		rt_kprintf("[5.8G_wifi] wifi set frame ckecksum error\r\n");
		return -RT_ERROR;
	}
	//心跳数据
	if(in_data[2] & 0x04)
	{
		return RT_EOK;
	}
	read_param(0, (int8_t *)&w_param, sizeof(struct save_param));
	//SSID
	if(in_data[2] & 0x01)
	{
		ssid_len = in_data[3];
		if(ssid_len > 20)
		{
			rt_kprintf("[5.8G_wifi] wifi set ssid len error\r\n");
			return -RT_ERROR;
		}
		w_param.wifi_ap.have_ssid_flag = 0x55aa55aa;
		rt_memset(w_param.wifi_ap.ssid, 0 ,sizeof(w_param.wifi_ap.ssid));
		rt_memcpy(w_param.wifi_ap.ssid, &in_data[4], ssid_len);
	}
	else
	{
		ssid_len = 0;
	}
	//PASSWORD
	if(in_data[2] & 0x02)
	{
		if(ssid_len > 0)
		{
			password_len = in_data[ssid_len + 4];
			password_point = &in_data[ssid_len + 5];
		}
		else
		{
			password_len = in_data[ssid_len+3];
			password_point = &in_data[ssid_len + 4];
		}
		if(password_len > 20)
		{
			rt_kprintf("[5.8G_wifi] wifi set password len error\r\n");
			return -RT_ERROR;
		}
		w_param.wifi_ap.have_password_flag = 0x55aa55aa;
		rt_memset(w_param.wifi_ap.password, 0 ,sizeof(w_param.wifi_ap.password));
		rt_memcpy(w_param.wifi_ap.password, password_point, password_len);
	}
	//写入参数
	write_param(0, (int8_t *)&w_param,sizeof(struct save_param));
	read_param(0, (int8_t *)&r_param, sizeof(struct save_param));
	if(rt_memcmp(&w_param, &r_param, sizeof(struct save_param)) == 0)
	{
		rt_kprintf("set info ok\r\n");
		generate_ask(ask_data, in_data[2], 1);
		rt_device_write(dev, 0, ask_data, ask_data[1]);
		rt_thread_delay(500);
		//rt_reboot();
	}
	else
	{
		generate_ask(ask_data, in_data[2], 0);
		rt_device_write(dev, 0, ask_data, ask_data[1]);
	}
	return RT_EOK;
}

#define WIFI_BUFFER_NUM 8
static rt_uint8_t ap_trans_buf[WIFI_BUFFER_NUM][1024] = {0};
static rt_uint8_t data_backup_buf[1024];
static rt_uint16_t data_backup_len = 0;
static rt_uint8_t ap_trans_buf_choise = 0;
extern bool get_ap_trans_flag(void);
rt_uint32_t wifi_recv_len = 0;
extern bool wifi_tick_flag ;
extern uint8_t connect_indicator_flag;
static uint8_t wifi_error_data[]={0x69,0x74,0x20,0x63,0x61,0x6e,0x27,0x74,0x20,0x66, \
                                  0x69,0x6e,0x64,0x20,0x41,0x54,0x20,0x66,0x6c,0x61,0x67,0x21,0x0a};

void app_wifi_recv_entry(void *arg){
	rt_uint16_t read_cnt = 0;
	rt_uint16_t buf_len = 0;
	rt_uint16_t index = 0;
	rt_uint8_t ret_flag = 0;
	rt_uint8_t tick_data[16] = {0};
	__IO rt_uint16_t data_len = 0;
	__IO bool re_recvd_data = false;
	uint16_t send_len = 0;
	
	static rt_uint16_t recv_tick_time_count = 0;
	static rt_uint16_t send_tick_time_count = 0;
	rt_device_t ap_trans_dev = RT_NULL;
	rt_device_t uwb_trans_dev = RT_NULL;
	ap_trans_dev = rt_device_find("uart6");
    if(ap_trans_dev == RT_NULL)
        rt_kprintf("[wifi] uart6 device not found\r\n");
    else
        rt_device_open(ap_trans_dev,RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_DMA_RX | RT_DEVICE_FLAG_DMA_TX);
	//打开uwb设备
	uwb_trans_dev = rt_device_find("uart1");
	if(uwb_trans_dev == RT_NULL)
		rt_kprintf("[wifi] uart1 device not found\r\n");
	else
		rt_device_open(uwb_trans_dev,RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_DMA_RX | RT_DEVICE_FLAG_DMA_TX);
	
	while(!get_ap_trans_flag())
	{
		rt_thread_delay(10);
	}
	rt_memset(ap_trans_buf[ap_trans_buf_choise], 0, buf_len);
	buf_len = sizeof(ap_trans_buf[0]);
	while(1){	
		while(wifi_running){
			//剩余数据进行拷贝
			if(data_backup_len > 0){
				rt_memcpy(ap_trans_buf[ap_trans_buf_choise], data_backup_buf, data_backup_len);
				data_len = data_backup_len;
				//清空备份buf
				data_backup_len = 0;
				rt_memset(data_backup_buf, 0, sizeof(data_backup_buf));
			}
			//接收数据
			read_cnt = rt_device_read(ap_trans_dev, 0 , ap_trans_buf[ap_trans_buf_choise]+data_len, buf_len-data_len);
			//rt_kprintf()
			if(read_cnt){
				data_len += read_cnt;
				wifi_recv_len += read_cnt;
			}
			else{
				if(data_len > 0){
					if(rt_memcmp(ap_trans_buf[ap_trans_buf_choise], wifi_error_data, sizeof(wifi_error_data)) != 0){
						for(index = 0; index < data_len; index++){
							ret_flag = inf_wifi_parse_char(ap_trans_buf[ap_trans_buf_choise][index]);
							if((ret_flag == 0x01) || (ret_flag == 0x02)){
								recv_tick_time_count = 0;
								connect_indicator_flag = 0x03;
								//拷贝剩余数据
								if((index+1) != data_len){
									data_backup_len = data_len - (index+1);
									rt_memcpy(data_backup_buf, ap_trans_buf[ap_trans_buf_choise]+(index+1), data_backup_len);
								}
								//得到数据及长度
								get_parse_buffer(ap_trans_buf[ap_trans_buf_choise],buf_len);
								get_parse_total_len(&send_len);
								if(ret_flag == 0x01){
									rt_device_write(uwb_trans_dev, 0 ,ap_trans_buf[ap_trans_buf_choise], send_len);
								}
								else if(ret_flag == 0x02 && wifi_tick_flag){
									wifi_tick_flag = false;
									//进行基站设置
									wifi_set_storage(ap_trans_buf[ap_trans_buf_choise],ap_trans_dev);
								}
								//切换数据buffer
								ap_trans_buf_choise++;
								if (ap_trans_buf_choise > (WIFI_BUFFER_NUM - 1)){
									ap_trans_buf_choise = 0;
								}
								break;
							}
						}
					}
					data_len = 0; 
					rt_memset(ap_trans_buf[ap_trans_buf_choise], 0, buf_len);
				}

			}
			/* 发送心跳数据帧 */
			if(send_tick_time_count++ >= 1000){
				send_tick_time_count = 0;
				rt_memset(tick_data , 0, sizeof(tick_data));
				generate_ask(tick_data, 0x04, 0x02);
				rt_device_write(ap_trans_dev, 0, tick_data, tick_data[1]);
			}
			if(recv_tick_time_count++ >= 3000){
				recv_tick_time_count = 0;
				connect_indicator_flag = 2;
			}
			rt_thread_delay(1);
		}
		rt_thread_delay(1);
	}
}


