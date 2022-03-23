#include "at_wifi.h"
#include "string.h"


#define AT_MAX(a,b) (a)>(b)?(a):(b)

#define EXIT_TRANS_CMD    "+++"
//读取操作的指令在末尾添加\r\n
#define READ_MODULE_VER  								"AT+VER?\r\n"//模块版本的指令
#define READ_WIFI_OPMODE 								"AT+OPMODE?\r\n"//获取模块的工作模式：1STA，2AP
#define READ_LINK_STATUS 								"AT+LINKSTATUS?\r\n"//读取link状态
#define DISCONNECT_WIFI_ROUTER					"AT+DISCONNECT\r\n"
#define READ_STA_SETTING         				"AT+STASETTING?\r\n"
#define READ_AP_SETTING         				"AT+APSETTING?\r\n"
#define READ_WIFI_WIRELESSMODE      		"AT+WIRELESSMODE?\r\n"
#define READ_STA_IP             				"AT+STAIP?\r\n"
#define READ_AP_IP              				"AT+APIP?\r\n"
#define READ_AP_LSIT              			"AT+APLIST?\r\n"
#define REBOOT                   				"AT+REBOOT\r\n"
#define ENTER_TRANS_CMD									"AT+TCPC_TRAN\r\n"
#define READ_TCP_CLIENT  								"AT+TCPCLIENT?\r\n"
#define READ_UDP_CLIENT									"AT+TCPCLIENT?\r\n"
#define SET_WIFI_OPMODE           			"AT+OPMODE="
#define SET_WIFI_WIRELESSMODE       		"AT+WIRELESSMODE="
#define SET_AP_INFO 										"AT+APSETTING="
#define SET_AP_IP_INFO              		"AT+APIP="
#define SET_TCP_AUTO_MODE								"AT+TCPS_AUTOTRAN="
#define ENTER_TRANS_MODE								"AT+TCPC_TRAN="
#define TCPC_CONNECT_CREATE       			"AT+TCPCLIENT="
#define TCPC_CONNECT_CLOSE       				"AT+TCPCLIENT="
#define UDPC_CONNECT_CREATE       			"AT+UDPCLIENT="
#define UDPC_CONNECT_CLOSE       				"AT+UDPCLIENT="
#define TCPC_AUTO_TRANS									"AT+TCPC_AUTOTRAN="
#define CONN_WIFI_ROUTER								"AT+STASETTING="
#define SET_STA_IP											"AT+STAIP="
#define PING_IP               					"AT+PING="

#define wifi_rst  GET_PIN(A,6)
static void wifi_hw_reset_enable(void){
	rt_pin_write(wifi_rst,PIN_HIGH);
	rt_thread_delay(50);
	rt_pin_write(wifi_rst,PIN_LOW);
	rt_thread_delay(50);
	rt_pin_write(wifi_rst,PIN_HIGH);
}
static void wifi_hw_reset_init(void){
	rt_pin_mode(wifi_rst,PIN_MODE_OUTPUT);
	rt_thread_delay(10);
	wifi_hw_reset_enable();
}

void wifi_device_init(int recv_size,char *dev_name){
	rt_uint32_t ret = RT_EOK;
	rt_uint8_t 	count = 0;
	do{
		ret = at_client_init(dev_name,recv_size);
		if(count++ > 5){
			count = 0;
			rt_kprintf("\r\n[wifi] init failed\r\n");
			rt_thread_delay(2000);
		}
	}while(ret != RT_EOK);
	at_set_end_sign('\n');
	wifi_hw_reset_init();
}

//wifi退出透传模式,调用此函数之前应挂起透传数据接收线程
void wifi_exit_trans_mode(void){
	at_exec_cmd(RT_NULL,EXIT_TRANS_CMD);
	rt_thread_resume(at_client_get_first()->parser);
}

int wifi_get_version(char *version){
	const char *buf;
	int ret = RT_NULL;
	if(version == RT_NULL){
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}
	
	at_response_t resp = at_create_resp(64,2,10000);
	if(at_exec_cmd(resp,READ_MODULE_VER) != RT_EOK){
		ret = -RT_ERROR;
		goto error;
	}
	
	buf = at_resp_get_line(resp,1);
	rt_kprintf("buf = %s\r\n",buf);
	if(buf == RT_NULL){
		rt_kprintf("[wifi] get  version data failed\r\n");
		ret = -RT_ERROR;
		goto error;
	}
	rt_enter_critical();
	memcpy(version,buf+5,strlen(buf) -4);
	rt_exit_critical();
	rt_kprintf("[5.8G_wifi] version is : %s\r\n",version);
	error:
	at_delete_resp(resp);
	return ret;
}

void app_at_wifi_entry(void *entry){
	char version[32];
	wifi_device_init(128,"uart6");
	wifi_exit_trans_mode();
	
	while(1){
		while(wifi_get_version(version) != RT_EOK){
			rt_thread_delay(100);
		}
	}
}
