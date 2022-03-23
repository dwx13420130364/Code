#include "stmflash.h"
#include "rtthread.h"
#include "finsh.h"
#include "stdbool.h"


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
	
	FlashEraseInit.TypeErase=FLASH_TYPEERASE_SECTORS;       //擦除类型，扇区擦除 
	FlashEraseInit.Sector=Flash_GetSector(Address);   		//要擦除的扇区
	FlashEraseInit.NbSectors=1;                             //一次只擦除一个扇区
	FlashEraseInit.VoltageRange=FLASH_VOLTAGE_RANGE_3;      //电压范围，VCC=2.7~3.6V之间!!
	if(HAL_FLASHEx_Erase(&FlashEraseInit,&SectorError)!=HAL_OK) 
	{
    	rt_kprintf("[FLASH]erase error\n");
	}
	
	HAL_FLASH_Lock();
}

void Flash_Write8BitDatas(uint32_t address, uint16_t length, int8_t* data_8)
{
    HAL_StatusTypeDef FlashStatus=HAL_OK;
	uint32_t endaddr=0;
	
 	HAL_FLASH_Unlock();             //解锁	
	endaddr=address+length;	//写入的结束地址
    
    FlashStatus=FLASH_WaitForLastOperation(FLASH_WAITETIME);            //等待上次操作完成
	if(FlashStatus == HAL_OK)
	{
		while(address < endaddr)//写数据
		{
            if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE,address,*data_8)!=HAL_OK)//写入数据
			{ 
				rt_kprintf("[FLASH]error write.");
				break;	//写入异常
			}
			address++;
			data_8++;
		} 
	}
	HAL_FLASH_Lock();           //上锁
}


void Flash_Write16BitDatas(uint32_t address, uint16_t length, int16_t* data_16)
{
	HAL_StatusTypeDef FlashStatus=HAL_OK;
	uint32_t endaddr=0;
	
 	HAL_FLASH_Unlock();             //解锁	
	endaddr=address+length*2;	//写入的结束地址
    
    FlashStatus=FLASH_WaitForLastOperation(FLASH_WAITETIME);            //等待上次操作完成
	if(FlashStatus == HAL_OK)
	{
		while(address < endaddr)//写数据
		{
            if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,address,*data_16)!=HAL_OK)//写入数据
			{ 
				rt_kprintf("[FLASH]error write.");
				break;	//写入异常
			}
			address += 2;
			data_16++;
		} 
	}
	HAL_FLASH_Lock();           //上锁
}

void Flash_Write32BitDatas(uint32_t address, uint16_t length, int32_t* data_32)
{
	HAL_StatusTypeDef FlashStatus=HAL_OK;
	uint32_t endaddr=0;
	
 	HAL_FLASH_Unlock();         //解锁	
	endaddr=address+length*4;		//写入的结束地址
    
    FlashStatus=FLASH_WaitForLastOperation(FLASH_WAITETIME);            //等待上次操作完成
	if(FlashStatus == HAL_OK)
	{
		while(address < endaddr)//写数据
		{
            if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,address,*data_32)!=HAL_OK)//写入数据
			{ 
				rt_kprintf("[FLASH]error write.");
				break;	//写入异常
			}
			address += 4;
			data_32++;
		} 
	}
	HAL_FLASH_Lock();           //上锁
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
		rt_kprintf("[stm_flash] write addr not mem align\r\n");
		return -RT_ERROR;
	}
	if((start_addr >= STM32_FLASH_END_ADDR) || (start_addr < STM32_FLASH_BASE_ADDR))
	{
		rt_kprintf("[stm_flash] write addr is not correct addr\r\n");
		return -RT_ERROR;
	}
	if(start_addr < FLASH_SAVE_ADDR)
	{
		rt_kprintf("[stm_flash] write addr is not user data domain\r\n");
		return -RT_ERROR;
	}
	if(end_addr >= STM32_FLASH_END_ADDR)
	{
		rt_kprintf("[stm_flash] write len is more than\r\n");
		return -RT_ERROR;
	}
	while(start_addr < end_addr)
	{
		if(STMFLASH_ReadWord(start_addr) != 0XFFFFFFFF)//有非0XFFFFFFFF的地方,要擦除这个扇区
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
			backup_data = rt_malloc(malloc_len);
			if(backup_data != NULL)
			{
				Flash_Read8BitDatas(FLASH_SAVE_ADDR,malloc_len,backup_data);
			}
			else
			{
				rt_kprintf("[stm_flash] malloc mem faield\r\n");
				return -RT_ERROR;
			}
		}
		Flash_EraseSector(start_addr);
		FlashStatus=FLASH_WaitForLastOperation(FLASH_WAITETIME);//等待上次操作完成
		if(FlashStatus == HAL_OK)
		{
			Flash_Write8BitDatas(FLASH_SAVE_ADDR,malloc_len,backup_data);
		}
		else
		{
			rt_kprintf("[stm_flash] write data faield\r\n");
			return -RT_ERROR;
		}
		HAL_FLASH_Lock();           //上锁
	}
	if(backup_data != NULL)
	{
		rt_free(backup_data);
	}
	return RT_EOK;	
}


void read_param(uint32_t add_off, int8_t* data_8, uint16_t length)
{
	Flash_Read8BitDatas(FLASH_SAVE_ADDR + add_off, length, data_8);
}

void write_param(uint32_t add_off, int8_t* data_8, uint16_t length)
{
	uint32_t start_addr = FLASH_SAVE_ADDR + add_off;
	uint32_t end_addr = FLASH_SAVE_ADDR + add_off + length;
	if(Flash_WriteAddrCheck(start_addr, end_addr) == RT_EOK)
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
	if(Flash_WriteAddrCheck(start_addr, end_addr) == RT_EOK)
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
	if(Flash_WriteAddrCheck(start_addr, end_addr) == RT_EOK)
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

