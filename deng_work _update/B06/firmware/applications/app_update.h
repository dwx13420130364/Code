#ifndef __APP_UPDATE_H
#define __APP_UPDATE_H


#include "stdint.h"
#pragma pack(1) 
typedef struct 
{
 	uint32_t lenth; 			//固件长度（字节数）
	uint16_t pkg_num;			//数据包数，每包数据512字节
 	uint8_t version[2];			//固件版本（例：1.1.3.0）
 	uint8_t md5[16];			//固件MD5
} firmware_heads_t;
#pragma pack() 

typedef struct 
{
	uint16_t pkg_id; 			//数据包序号
 	uint8_t data[850]; 			//数据
} firmware_datas_t;   


void firmware_update_main(void* parameter);

#endif

