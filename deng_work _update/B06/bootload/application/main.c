#include "stdbool.h"
#include "bsp_led.h"
#include "stm32f4xx.h"
#include "drv_usart.h"
#include "stmflash.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "md5.h"
#include "param.h"

#define APPLICATION_ADDRESS     		(ADDR_FLASH_SECTOR_2)
#define FLASH_BIN_SAVE_INFO_ADDR		(ADDR_FLASH_SECTOR_1)
#define FLASH_BIN_CONTENT_ADDR			(ADDR_FLASH_SECTOR_5)
#define BIN_NEED_UPDATE    				0x55AA55AA
#define BIN_UPDATE_SUCCESS    			0xAA55AA55

typedef  void (*iap_fun)(void);
iap_fun jump_into_app; 
struct save_param s_param;	//�洢��Ϣ

static uint8_t bin_buf[1024];
void hg_show_version(void)
{
	printf("\r\n    __  ___       __       ______                __");
	printf("\r\n   / / / (_)___ _/ /_     / ____/_______  ____ _/ /_");
	printf("\r\n  / /_/ / / __ `/ __ \\   / / __/ ___/ _ \\/ __ `/ __/");
	printf("\r\n / __  / / /_/ / / / /  / /_/ / /  /  __/ /_/ / /_");
	printf("\r\n/_/ /_/_/\\__, /_/ /_/   \\____/_/   \\___/\\__,_/\\__/");
	printf("\r\n        /____/");
	printf("\r\n[HG B02 bootloader]V1.0.0");
}

void reboot(void)
{
    NVIC_SystemReset();
}


static void SystemClock_Config(void)
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_OscInitTypeDef RCC_OscInitStruct;
    HAL_StatusTypeDef ret = HAL_OK;

	__HAL_RCC_PWR_CLK_ENABLE();
	
    /* Enable HSE Oscillator and activate PLL with HSE as source */
	RCC_OscInitStruct.OscillatorType =  RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.LSIState = RCC_LSI_ON;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 336;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
	RCC_OscInitStruct.PLL.PLLQ = 7;

    ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
    if(ret != HAL_OK)
    {
        while (1) { ; }
    }

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
       clocks dividers */
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |\
                                   RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
    if (ret != HAL_OK)
    {
        while (1) { ; }
    }
}

void md5IntToChar(uint8_t inBuf[16])
{
	uint8_t num0 = 0, num1 = 0;
	for(uint8_t i=0;i<16;i++)
	{
		num0 = inBuf[i]/16;
		
		if (num0 < 10)
			num0 += 0x30;
		else
			num0 += 0x37;
		
		num1 = inBuf[i]%16;
		if (num1 < 10)
			num1 += 0x30;
		else
			num1 += 0x37;
		
		//printf("%c",num0);
		//printf("%c",num1);
	}
}



static bool readBinFileAndWriteFile(uint32_t read_addr, uint32_t write_addr, struct bin_info *bin)
{
	if(bin == NULL)
		return false;
	md5Ctx_t context;
	uint8_t md5_zero[16] = {0};
	uint8_t cal_md5[16];
	uint32_t readPackets = 0;
	uint32_t read_remain_len = 0;
	uint32_t buf_len = sizeof(bin_buf);
	memset(bin_buf, 0, buf_len);
	
	//�����ȡ����	
	readPackets = bin->lenth / buf_len;
	read_remain_len = bin->lenth % buf_len;
	
	__IO uint32_t readBlockAddr = read_addr;
	__IO uint32_t WriteBlockAddr = write_addr;
	//printf("\r\n updating");
	//��ʼ��MD5ֵ
	 MD5Init(&context);
	//�����ļ�д��
	for(uint16_t i=0;i<readPackets;i++)
	{
		//printf("-");
		Flash_Read8BitDatas(readBlockAddr, buf_len,(int8_t *)bin_buf);
		Flash_Write8BitDatas(WriteBlockAddr,buf_len,(int8_t *)bin_buf);
		readBlockAddr += buf_len;
		WriteBlockAddr += buf_len;
	}
	//printf("-");
	//д��ʣ������
	if(read_remain_len > 0)
	{
		Flash_Read8BitDatas(readBlockAddr, buf_len,(int8_t *)bin_buf);
		Flash_Write8BitDatas(WriteBlockAddr,buf_len,(int8_t *)bin_buf);
		readBlockAddr += read_remain_len;
		WriteBlockAddr += read_remain_len;
	}
	//printf("--update end");
	//��ȡд���ļ��ȶ�
	readBlockAddr = write_addr;
	for(uint16_t i=0;i<readPackets;i++)
	{
		Flash_Read8BitDatas(readBlockAddr, buf_len,(int8_t *)bin_buf);
		readBlockAddr += buf_len;
		//����MD5����
		MD5Update(&context,bin_buf,buf_len);
	}
	//��ȡʣ������
	if(read_remain_len > 0)
	{
		Flash_Read8BitDatas(readBlockAddr, buf_len,(int8_t *)bin_buf);
		readBlockAddr += read_remain_len;
		//����MD5����
		MD5Update(&context,bin_buf,read_remain_len);
	}

	//���MD5
	MD5Final(&context, cal_md5);
	//��ӡmd5ֵ
	md5IntToChar(cal_md5);
	if((memcmp(cal_md5,bin->md5,sizeof(cal_md5)) == 0) && (memcmp(cal_md5, md5_zero, sizeof(cal_md5))))
	{
		//�����ɹ�
		return true;
	}
	else
	{
		//����ʧ��
		return false;
	}
}


void iap_jump_into_app(uint32_t app_addr)
{
	//���ջ����ַ�Ƿ�Ϸ�.0x2FFE0000
	if(((*(__IO uint32_t*)app_addr)&0x2FFC0000)==0x20000000)	
	{
		HAL_Delay(100);
		//�û��������ڶ�����Ϊ����ʼ��ַ(��λ��ַ)		
		jump_into_app = (iap_fun)(*(__IO uint32_t*)(app_addr + 4));
		//��תǰ�ر����п������ж�
		__set_PRIMASK(1);
		//����ջ��ָ�� 
		__set_MSP(*(__IO uint32_t*) app_addr);
		//��ת��APP.
		jump_into_app();									
	}
}

int main(void)
{
	bool update_flag = false;

    HAL_Init();
	//ϵͳʱ�����ã�ѡ���ⲿʱ�ӣ���Ƶ��84MHZ
    SystemClock_Config();
	HAL_SYSTICK_Config(SystemCoreClock / 1000);
	//��������
	stm32_uart_init();
	hg_show_version();
	//��ʼ��led����ָʾ����״̬
	power_led_init();
	indicator_led_init();
	//��ȡ������Ϣ
	memset(&s_param, 0, sizeof(struct save_param));
	read_param(0,(int8_t *)&s_param,sizeof(struct save_param));
	//��ӡ�汾��Ϣ
	//printf("\r\n[HG B02 bootloader]V1.0.0");
	//bin�ļ���Ҫ����
	if(s_param.bin_head.updateFlag == BIN_NEED_UPDATE)
	{
		if((s_param.bin_head.lenth > 0) && (s_param.bin_head.lenth < 0xffffffff))
		{
			//��ȡ�ļ���д�뵽flash��
			Flash_EraseSector(ADDR_FLASH_SECTOR_2);
			Flash_EraseSector(ADDR_FLASH_SECTOR_3);
			Flash_EraseSector(ADDR_FLASH_SECTOR_4);
			update_flag = readBinFileAndWriteFile(FLASH_BIN_CONTENT_ADDR, APPLICATION_ADDRESS, &s_param.bin_head);
			//�����ļ���Ϣ
			if(update_flag == true)
			{
				s_param.bin_head.updateFlag = BIN_UPDATE_SUCCESS;
				write_param(0,(int8_t *)&s_param,sizeof(struct save_param));
			}
			reboot();
		}
	}
	while(1)
	{

		
		if((s_param.bin_head.updateFlag == BIN_UPDATE_SUCCESS) ||\
 		   ((s_param.bin_head.updateFlag == 0xffffffff)&&(s_param.bin_head.lenth == 0xffffffff)))
		{
		//	printf("jinlaile\r\n");
			//��ת��IAP
			iap_jump_into_app(APPLICATION_ADDRESS);
		}
		else
		{
			indicator_led_enable();
			power_led_disable();
			HAL_Delay(500);
			power_led_enable();
			indicator_led_disable();
			HAL_Delay(500);
		}
	}
}
