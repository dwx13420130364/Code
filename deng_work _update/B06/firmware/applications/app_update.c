#include "app_update.h"
#include "rtthread.h"
#include "app_md5.h"
#include "stdbool.h"
#include "string.h"
#include "stmflash.h"
#include "app_uwb.h"
#include "param.h"
#include "checksum.h"

#define FIREWARE_BUF_LEN 		900
#define FIREWARE_BUF_NUM 		5
#define BIN_NEED_UPDATE 		0X55AA55AA
#define BIN_INFO_SAVE_ADDR 	256
rt_mq_t firmware_update;
__IO uint8_t send_fly_reboot_flag = 0;
static uint8_t update_buf[FIREWARE_BUF_LEN];

extern void reboot(void);

void md5IntToChar(rt_uint8_t inBuf[16]){
	rt_uint8_t num0 = 0,num1 = 0;
	rt_uint8_t buf[33] = {0};
	rt_uint8_t j = 0;
	for(rt_uint8_t i = 0;i<16;i++){
		num0 = inBuf[i]/16;
		if(num0 < 10)num0 += 0x30;
		else         num0 += 0x37;
		num1 = inBuf[i]%16;
		if(num1 < 10)num1 += 0x30;
		else         num1 += 0x37;
		
		buf[j++] = num0;
		buf[j++] = num1;
	}
	rt_kprintf("%s\r\n",buf);
}

bool firmware_md5_cal(rt_uint32_t start_addr,rt_uint32_t firmware_lenth, rt_uint8_t *md5)
{
	rt_uint8_t md5_zero[16] = {0};
	rt_uint8_t cal_md5[16] = {0};
	rt_uint8_t read_buf[1024];
	rt_uint16_t readPackets = 0;
	rt_uint16_t read_remain_len = 0;
	//计算读取包数
	rt_uint16_t buf_len = sizeof(read_buf);
	readPackets = firmware_lenth / buf_len;
	read_remain_len = firmware_lenth % buf_len;
	rt_kprintf("len:%d,pkg_num:%d,re:%d\r\n",firmware_lenth,readPackets,read_remain_len);
	__IO rt_uint32_t readBlockAddr = start_addr;
	md5Ctx_t firmware_context = {0};
	MD5Init(&firmware_context);
	for(rt_uint16_t i=0; i<readPackets; i++)
	{
		rt_memset(read_buf, 0, buf_len);
		Flash_Read8BitDatas(readBlockAddr, buf_len,(int8_t *)read_buf);
		readBlockAddr += buf_len;
		//跟新MD5内容
		MD5Update(&firmware_context, read_buf, buf_len);
	}
	//读取剩余数据
	if(read_remain_len > 0)
	{
		rt_memset(read_buf, 0, buf_len);
		Flash_Read8BitDatas(readBlockAddr, read_remain_len,(int8_t *)read_buf);
		readBlockAddr += read_remain_len;
		//跟新MD5内容
		MD5Update(&firmware_context, read_buf, read_remain_len);

	}

	//输出MD5
	MD5Final(&firmware_context, cal_md5);
	rt_kprintf("cal md5   :");
	md5IntToChar(cal_md5);
	rt_kprintf("server md5:");
	md5IntToChar(md5);
	if((memcmp(md5,cal_md5,sizeof(cal_md5)) == 0) && (memcmp(cal_md5, md5_zero, sizeof(cal_md5))))
	{
		return true;
	}
	else
	{
		return false;
	}
}

int firmware_info_write_to_falsh(firmware_heads_t * firmware_head){
	struct save_param w_param;
	struct save_param r_param;
	
	//读取参数，赋值需要修改参数
	read_param(0,(int8_t *)&w_param,sizeof(struct save_param));
	rt_memcpy(&w_param.bin_head,firmware_head,sizeof(firmware_heads_t));
	w_param.bin_head.updateFlag = 0x55aa55aa;
	write_param(0,(int8_t *)&w_param, sizeof(struct save_param));
	read_param(0, (int8_t *)&r_param, sizeof(struct save_param));
	if(rt_memcmp(&w_param,&r_param,sizeof(struct save_param)) == 0){
		rt_kprintf("[BIN] save bin info ok\r\n");
		return RT_EOK;
	}
	return RT_FALSE;
}

rt_uint32_t version_conversion(rt_uint8_t endian,rt_uint8_t *data)
{
	//小端格式
	if(endian == 0x01)
	{
		return data[0]<<8|data[1];
	}
	return data[1]<<8|data[0];
}

void new_md5IntToChar(rt_uint8_t inBuf[16],char buf[33]){
	rt_uint8_t num0 = 0,num1 = 0;

	rt_uint8_t j = 0;
	for(rt_uint8_t i = 0;i<16;i++){
		num0 = inBuf[i]/16;
		if(num0 < 10)num0 += 0x30;
		else         num0 += 0x37;
		num1 = inBuf[i]%16;
		if(num1 < 10)num1 += 0x30;
		else         num1 += 0x37;
		
		buf[j++] = num0;
		buf[j++] = num1;
	}
	rt_kprintf("%s\r\n",buf);
}


bool check_update_bin_file_md5(rt_uint32_t start_addr,rt_uint32_t firmware_lenth, rt_uint8_t *md5){
	rt_uint8_t md5_zero[33] = {0};
	char cal_md5[33] = {0};
	char update_bin_md5[33];
	
	get_bin_md5(start_addr,firmware_lenth,cal_md5);
	rt_kprintf("check md5:%s\r\n",cal_md5);
	new_md5IntToChar(md5,update_bin_md5);
	rt_kprintf("recv md5:%s\r\n",update_bin_md5);
	if((memcmp(update_bin_md5,cal_md5,sizeof(cal_md5)) == 0) && (memcmp(cal_md5, md5_zero, sizeof(cal_md5))))
	{
		rt_memset(update_bin_md5,0,33);
		rt_memset(cal_md5,0,33);
		return true;
	}
	else
	{
		return false;
	}
}


extern bool wifi_running ;
rt_uint8_t read_buff[1024];
char read_md5_buf[200];
extern void set_ap_trans_falg(bool flag);
rt_uint8_t update_led = 0;
extern rt_uint8_t led_change_num;
extern rt_uint8_t uwb_update_flag;
extern bool ap_init_flag;
void app_update_entry(void *arg){
	rt_uint32_t firmware_bitmap[60];
	rt_uint32_t firmware_pkg_lenth = 0;
	rt_uint16_t firmware_pkg_id = 0;
	rt_uint16_t firmware_id_count = 0;
	rt_uint16_t data_frame_len = 0;
	rt_uint8_t  update_flag = 0;			//状态：2：发送固件请求，1：升级中，0：未进行升级
	rt_uint8_t  info_checking = 0;
	rt_uint32_t firmware_version = 0;
	rt_uint32_t hg_version = 0;
	firmware_heads_t firmware_head;


	while(firmware_update == NULL){
		firmware_update = rt_mq_create("firmware",FIREWARE_BUF_LEN,FIREWARE_BUF_NUM,RT_IPC_FLAG_FIFO);
		rt_thread_delay(10);
	}
	rt_kprintf("\r\n[UPDATE]firmware mq creat ok\r\n");
	rt_memset(update_buf, 0, sizeof(update_buf));
	while(1){
		if(rt_mq_recv(firmware_update,update_buf,FIREWARE_BUF_LEN,0) == RT_EOK){
			uwb_update_flag = 0x01;
			for(uint16_t i = 0;i<FIREWARE_BUF_LEN;i++){
			
				if(update_buf[i] == 0xcc && update_buf[i+1] == 0x02){
						update_led = 1;
					if(info_checking==0){
						info_checking = 1;
							//固件正在升级中，重新接收到固件信息
							if(update_flag == 1){
								firmware_id_count = 0;
							
								update_flag = 0;
							}
							rt_memset(&firmware_head,0,sizeof(firmware_head));
							rt_memcpy(&firmware_head,&update_buf[i+4],sizeof(firmware_head));
							rt_kprintf("lenth:%d,pkg_num:%d\r\n",firmware_head.lenth,firmware_head.pkg_num);
							if(firmware_head.lenth > (128*1024)){
								info_checking = 0;
								rt_kprintf("[BIN] firmware len too long\r\n");
								break;
							}
							firmware_version = version_conversion(0,firmware_head.version);
							hg_version = HG_VERSION*1000 + HG_SUBVERSION*100 + HG_REVISION*10+HG_BETA;
							rt_kprintf("old version:%d,new:%d\r\n",hg_version,firmware_version);
							if(firmware_version < hg_version -50)
							{
								info_checking = 0;
								rt_kprintf("[BIN] version low\r\n");
								led_change_num = 0x00;
								update_flag = 0x00;
								break;
							}
							if(firmware_md5_cal(ADDR_FLASH_SECTOR_2,firmware_head.lenth, firmware_head.md5) == true)
							{
								info_checking = 0;
								rt_kprintf("\n[BIN]save firmware ok\r\n");
								update_flag = 0x00;
								led_change_num = 0x00;
								
								break;
									
							}
							//需要升级，擦除扇区,次数不能写入数据，不能保证接收文件准确
							rt_kprintf("[BIN]erase sector5\r\n");
							Flash_EraseSector(ADDR_FLASH_SECTOR_5);
							rt_memset(firmware_bitmap, 0, sizeof(firmware_bitmap));
							firmware_id_count = 0;
							update_flag = 0x01;
							rt_kprintf("wait update\r\n");
						}
					}
				if(update_buf[i] == 0xcc && update_buf[i+1] == 0x03){	
						if(update_flag){
							update_led ^= 1;
						}
					data_frame_len = update_buf[i+2]+update_buf[i+3] * 256;
					firmware_pkg_id = update_buf[i+4] + update_buf[i+5] *256;
					if(firmware_pkg_id < firmware_head.pkg_num){
						firmware_pkg_lenth = data_frame_len - 7;
							
					}
					if(firmware_pkg_id == firmware_head.pkg_num){
						firmware_pkg_lenth = firmware_head.lenth - (firmware_head.pkg_num-1)*850;
					}
					if((update_flag == 1) && (firmware_pkg_id <= firmware_head.pkg_num) && \
							 ((firmware_bitmap[(firmware_pkg_id)/32] & (1<<((firmware_pkg_id)%32)))==0)){
										
						firmware_id_count = firmware_pkg_id;
						//标志当前数据包已经接收完成
						firmware_bitmap[(firmware_pkg_id)/32] |= (1<<(firmware_pkg_id)%32);
						rt_enter_critical();
						Flash_Write8BitDatas(ADDR_FLASH_SECTOR_5+(firmware_pkg_id-1)*850, firmware_pkg_lenth, (int8_t*)&update_buf[i+6]);
						rt_exit_critical();
						rt_kprintf("write num :%d,%d,firmware_pkg_lenth : %d\r\n",firmware_pkg_id,firmware_id_count,firmware_pkg_lenth);
					}				
				}
			if((update_flag == 1) &&  (firmware_id_count == firmware_head.pkg_num)){
				
				rt_kprintf("\r\nend update\r\n");
				rt_kprintf("[BIN]receiver finish,start to check md5\r\n");
				if(firmware_md5_cal(ADDR_FLASH_SECTOR_5,firmware_head.lenth,firmware_head.md5) == true){
					update_led = 0;
					rt_kprintf("[BIN]save firmware ok,reboot after 5s\r\n");
				
					//接收完成，写入信息到flash中
					if(firmware_info_write_to_falsh(&firmware_head) == RT_EOK){
							rt_thread_delay(5000);
							reboot();
						}	
					}
					else{
						rt_kprintf("[BIN] md5 error\r\n");
						update_led = 1; 
						rt_thread_delay(3000);
						led_change_num = 0x00;	
						wifi_running = true;
						ap_init_flag = false;
					}
					firmware_id_count = 0;
					info_checking = 0;
					update_flag = 0;
					rt_memset(firmware_bitmap, 0, sizeof(firmware_bitmap));
				}

			}			
			rt_memset(update_buf, 0 ,FIREWARE_BUF_LEN);
		}	
		rt_thread_delay(1);
	}
}

