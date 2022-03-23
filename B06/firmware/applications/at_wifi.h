#ifndef __AT_WIFI_H
#define __AT_WIFI_H

#include "at.h"
#include "rtthread.h"
#include "rtdevice.h"
#include "board.h"

//WIFI上线标识
typedef enum{
	online_init,
	online_send,
	online_finish,
	online_success,
}online_status;

struct ip_info
{
	char status[7];
	char local_ip[16];
	char netmask[16];
	char gataway[16];
	char server_ip[16];
	char start_end_pool[2][16];	//0：开始IP,1：结束IP
	char local_port[6];
	char server_port[6];
};
typedef struct ip_info* ip_info_t;

struct ap_mode
{		
	char ssid[32];
	char bssid[32];
	char channel[4];
	char author_mode[16];//OPEN,WPA_PSK,WPA2_PSK,WPA_PSK_WPA2_PSK,AUTHOR_MODE_END
	struct ip_info ap_ip;
};

struct sta_mode
{
	char sock_num;
	char ssid[32];
	char password[32];
	char author_mode[16];//OPEN,WPA_PSK,WPA2_PSK,WPA_PSK_WPA2_PSK,AUTHOR_MODE_END
	struct ip_info sta_ip;
};
typedef struct sta_mode* sta_mode_t;
struct tcp_info
{
	char tcp_status;//0----close,1-----create;
	char sock_num;
	char server_ip[16];
	char port[6];
};


typedef struct tcp_info * tcp_info_t;
struct wifi_dev
{
	struct at_client client;
	rt_thread_t thread[2];		//[0]STA模式,[1]ap模式
	char module_ver[32];
	char mode;					//1:STA模式，2：AP模式
	char linkstatus;
	char trans_status;
	struct sta_mode sta;
	struct ap_mode	ap;
};
typedef struct wifi_dev* wifi_dev_t;

typedef enum{
	B_G_MIXED_2 = 0,
	B_ONLY_2,
	A_ONLY_5,
	A_B_G_MIXED_2,
	G_ONLY_2,
	A_B_G_N_MIXED_2_5,
	N_ONLY_2,
	G_N_MIXED_2,
	A_N_MIXED_5,
	B_G_N_MIXED_2,
	AGN_MIXED_2_5,
	N_ONLY_5,
}WireLessMode;




#endif
