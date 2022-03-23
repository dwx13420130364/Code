#include "stmflash.h"
#include "stdbool.h"
#include "stdlib.h"
#include "stdio.h"

uint16_t Flash_GetSector(uint32_t Address)
{
	uint16_t sector = 0;
	if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
	{
		sector = FLASH_SECTOR_0;
	}
	else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
	{
		sector = FLASH_SECTOR_1;
	}
	else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
	{
		sector = FLASH_SECTOR_2;
	}
	else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
	{
		sector = FLASH_SECTOR_3;
	}
	else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
	{
		sector = FLASH_SECTOR_4;
	}
	else /* (Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_5) */
	{
		sector = FLASH_SECTOR_5;
	}
	return sector;
}

uint32_t STMFLASH_ReadWord(uint32_t faddr)
{
	return *(__IO uint32_t *)faddr; 
}

void Flash_Read8BitDatas(uint32_t address, uint16_t length, int8_t* data_8)
{
	uint16_t i;
	for (i = 0; i<length; i++)
	{
		data_8[i] = *(__IO int8_t*)address;
		address++;
	}
}

void Flash_Read16BitDatas(uint32_t address, uint16_t length, int16_t* data_16)
{
	uint8_t i;
	for (i = 0; i<length; i++)
	{
		data_16[i] = *(__IO int16_t*)address;
		address = address + 2;
	}
}

void Flash_Read32BitDatas(uint32_t address, uint16_t length, int32_t* data_32)
{
	uint16_t i;
	for (i = 0; i<length; i++)
	{
		data_32[i] = *(__IO int32_t*)address;
		address = address + 4;
	}
}


void Flash_EraseSector(uint32_t Address)
{
	FLASH_EraseInitTypeDef FlashEraseInit;
	uint32_t SectorError=0;
	
	HAL_FLASH_Unlock();
	
	FlashEraseInit.TypeErase=FLASH_TYPEERASE_SECTORS;       //�������ͣ��������� 
	FlashEraseInit.Sector=Flash_GetSector(Address);   		//Ҫ����������
	FlashEraseInit.NbSectors=1;                             //һ��ֻ����һ������
	FlashEraseInit.VoltageRange=FLASH_VOLTAGE_RANGE_3;      //��ѹ��Χ��VCC=2.7~3.6V֮��!!
	if(HAL_FLASHEx_Erase(&FlashEraseInit,&SectorError)!=HAL_OK) 
	{
    	//printf("[FLASH]erase error\n");
	}
	
	HAL_FLASH_Lock();
}

void Flash_Write8BitDatas(uint32_t address, uint16_t length, int8_t* data_8)
{
    HAL_StatusTypeDef FlashStatus=HAL_OK;
	uint32_t endaddr=0;
	
 	HAL_FLASH_Unlock();             //����	
	endaddr=address+length;	//д��Ľ�����ַ
    
    FlashStatus=FLASH_WaitForLastOperation(FLASH_WAITETIME);            //�ȴ��ϴβ������
	if(FlashStatus == HAL_OK)
	{
		while(address < endaddr)//д����
		{
            if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE,address,*data_8)!=HAL_OK)//д������
			{ 
				//printf("[FLASH]error write.");
				break;	//д���쳣
			}
			address++;
			data_8++;
		} 
	}
	HAL_FLASH_Lock();           //����
}


void Flash_Write16BitDatas(uint32_t address, uint16_t length, int16_t* data_16)
{
	HAL_StatusTypeDef FlashStatus=HAL_OK;
	uint32_t endaddr=0;
	
 	HAL_FLASH_Unlock();             //����	
	endaddr=address+length*2;	//д��Ľ�����ַ
    
    FlashStatus=FLASH_WaitForLastOperation(FLASH_WAITETIME);            //�ȴ��ϴβ������
	if(FlashStatus == HAL_OK)
	{
		while(address < endaddr)//д����
		{
            if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,address,*data_16)!=HAL_OK)//д������
			{ 
				//printf("[FLASH]error write.");
				break;	//д���쳣
			}
			address += 2;
			data_16++;
		} 
	}
	HAL_FLASH_Lock();           //����
}

void Flash_Write32BitDatas(uint32_t address, uint16_t length, int32_t* data_32)
{
	HAL_StatusTypeDef FlashStatus=HAL_OK;
	uint32_t endaddr=0;
	
 	HAL_FLASH_Unlock();         //����	
	endaddr=address+length*4;		//д��Ľ�����ַ
    
    FlashStatus=FLASH_WaitForLastOperation(FLASH_WAITETIME);            //�ȴ��ϴβ������
	if(FlashStatus == HAL_OK)
	{
		while(address < endaddr)//д����
		{
            if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,address,*data_32)!=HAL_OK)//д������
			{ 
				//printf("[FLASH]error write.");
				break;	//д���쳣
			}
			address += 4;
			data_32++;
		} 
	}
	HAL_FLASH_Lock();           //����
}

int8_t Flash_WriteAddrCheck(uint32_t start_addr,uint32_t end_addr)
{
	int8_t *backup_data = NULL;
	int8_t need_copy = 0;
	uint32_t start_address = start_addr;
	uint32_t malloc_len = 0;
	HAL_StatusTypeDef FlashStatus=HAL_OK;
	if(start_addr%4 != 0)
	{
		//printf("[stm_flash] write addr not mem align\r\n");
		return -1;
	}
	if((start_addr >= STM32_FLASH_END_ADDR) || (start_addr < STM32_FLASH_BASE_ADDR))
	{
		//printf("[stm_flash] write addr is not correct addr\r\n");
		return -1;
	}
	if(end_addr >= STM32_FLASH_END_ADDR)
	{
		//printf("[stm_flash] write len is more than\r\n");
		return -1;
	}
	while(start_addr < end_addr)
	{
		if(STMFLASH_ReadWord(start_addr) != 0XFFFFFFFF)//�з�0XFFFFFFFF�ĵط�,Ҫ�����������
		{
			need_copy = 0x01;
			break;
		}
		else
		{
			start_addr += 4;
		}
	}
	start_addr = start_address;
	if(need_copy == 0x01)
	{
		HAL_FLASH_Unlock();
		malloc_len = start_addr - FLASH_SAVE_ADDR;
		if(malloc_len > 0)
		{
			backup_data = malloc(malloc_len);
			if(backup_data != NULL)
			{
				Flash_Read8BitDatas(FLASH_SAVE_ADDR,malloc_len,backup_data);
			}
			else
			{
				//printf("[stm_flash] malloc mem faield\r\n");
				return -1;
			}
		}
		Flash_EraseSector(start_addr);
		FlashStatus=FLASH_WaitForLastOperation(FLASH_WAITETIME);//�ȴ��ϴβ������
		if(FlashStatus == HAL_OK)
		{
			Flash_Write8BitDatas(FLASH_SAVE_ADDR,malloc_len,backup_data);
		}
		else
		{
			//printf("[stm_flash] write data faield\r\n");
			return -1;
		}
		HAL_FLASH_Lock();           //����
	}
	if(backup_data != NULL)
	{
		free(backup_data);
	}
	return 0;	
}


void read_param(uint32_t add_off, int8_t* data_8, uint16_t length)
{
	Flash_Read8BitDatas(FLASH_SAVE_ADDR + add_off, length, data_8);
}

void write_param(uint32_t add_off, int8_t* data_8, uint16_t length)
{
	uint32_t start_addr = FLASH_SAVE_ADDR + add_off;
	uint32_t end_addr = FLASH_SAVE_ADDR + add_off + length;
	if(Flash_WriteAddrCheck(start_addr, end_addr) == 0)
	{
		Flash_Write8BitDatas(FLASH_SAVE_ADDR + add_off, length, data_8);
	}	
}


void read_param16(uint32_t add_off, int16_t* data_16, uint16_t length)
{
	Flash_Read16BitDatas(FLASH_SAVE_ADDR + add_off, length, data_16);
}

void write_param16(uint32_t add_off, int16_t* data_16, uint16_t length)
{
	uint32_t start_addr = FLASH_SAVE_ADDR + add_off;
	uint32_t end_addr = FLASH_SAVE_ADDR + add_off + length*2;
	if(Flash_WriteAddrCheck(start_addr, end_addr) == 0)
	{
		Flash_Write16BitDatas(FLASH_SAVE_ADDR + add_off, length, data_16);
	}
}

void read_param32(uint32_t add_off, int32_t* data_32, uint16_t length)
{
	Flash_Read32BitDatas(FLASH_SAVE_ADDR + add_off, length, data_32);
}

void write_param32(uint32_t add_off, int32_t* data_32, uint16_t length)
{
	uint32_t start_addr = FLASH_SAVE_ADDR + add_off;
	uint32_t end_addr = FLASH_SAVE_ADDR + add_off + length*4;
	if(Flash_WriteAddrCheck(start_addr, end_addr) == 0)
	{
		Flash_Write32BitDatas(FLASH_SAVE_ADDR + add_off, length, data_32);
	}
}

void stm_flash_unlock(void)
{
    HAL_FLASH_Unlock();
}

void stm_flash_lock(void)
{
    HAL_FLASH_Lock();
}
