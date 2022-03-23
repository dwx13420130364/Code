#ifndef __PARAM_H
#define __PARAM_H

#include "stdint.h"

struct ap_info
{
	uint32_t have_ssid_flag;
	uint8_t ssid[20];
	uint32_t have_password_flag;
	uint8_t password[20];
};

struct sn_info
{
	uint32_t have_sn_flag;
	uint8_t sn[20];
};

#pragma pack(1)                 
struct bin_info
{
	uint32_t lenth; 			//�̼����ȣ��ֽ�����
	uint16_t pkg_num;			//���ݰ�����ÿ������512�ֽ�
 	uint8_t version[2];			//�̼��汾������1.1.3.0��
 	uint8_t md5[16];			//�̼�MD5
	uint32_t updateFlag;		//���±�־λ
};
#pragma pack()

struct save_param
{
	struct ap_info wifi_ap;			//wifi ap��Ϣ
	struct sn_info bs_sn;			//��վsn��
	struct bin_info bin_head;		//�̼�ͷ����Ϣ
};

#endif

