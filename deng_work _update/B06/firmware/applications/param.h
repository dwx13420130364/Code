#ifndef __PARAM_H
#define __PARAM_H

#include "stdint.h"
#include "rtthread.h"

struct ap_info
{
	rt_uint32_t have_ssid_flag;
	rt_uint8_t ssid[20];
	rt_uint32_t have_password_flag;
	rt_uint8_t password[20];
};

struct sn_info
{
	rt_uint32_t have_sn_flag;
	rt_uint8_t sn[20];
};


#pragma pack(1)                 
struct bin_info
{
	rt_uint32_t lenth; 			//固件长度（字节数）
	rt_uint16_t pkg_num;			//数据包数，每包数据512字节
 	rt_uint8_t version[2];			//固件版本（例：1.1.3.0）
 	rt_uint8_t md5[16];			//固件MD5
	rt_uint32_t updateFlag;		//跟新标志位
};
#pragma pack()

struct save_param
{
	struct ap_info wifi_ap;			//wifi ap信息
	struct sn_info bs_sn;			//基站sn号
	struct bin_info bin_head;		//固件头部信息
};

#endif


