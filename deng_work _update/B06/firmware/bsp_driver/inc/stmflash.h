#ifndef __STMFLASH_H__
#define __STMFLASH_H__
#include "stm32f4xx.h"

/* Base address of the Flash sectors */
#define STM32_FLASH_BASE_ADDR 	(0x08000000) 	//STM32 FLASH的起始地址
#define STM32_FLASH_END_ADDR	(0x08000000+256*1024)		
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbytes */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base @ of Sector 1, 16 Kbytes */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base @ of Sector 2, 16 Kbytes */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800c000) /* Base @ of Sector 3, 16 Kbytes */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbytes */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbytes */

#define FLASH_SAVE_ADDR 		ADDR_FLASH_SECTOR_1	//用户起始地址，暂定为第5个扇区的起始地址
#define FLASH_WAITETIME  50000


void Flash_EraseSector(uint32_t Address);
void Flash_Read8BitDatas(uint32_t address, uint16_t length, int8_t* data_8);
void Flash_Write8BitDatas(uint32_t address, uint16_t length, int8_t* data_8);
void Flash_Write16BitDatas(uint32_t address, uint16_t length, int16_t* data_16);
void Flash_Write32BitDatas(uint32_t address, uint16_t length, int32_t* data_32);
void read_param(uint32_t add_off, int8_t* data_8, uint16_t length);
void write_param(uint32_t add_off, int8_t* data_8, uint16_t length);
void read_param32(uint32_t add_off, int32_t* data_32, uint16_t length);
void write_param32(uint32_t add_off, int32_t* data_32, uint16_t length);	
void clear_param(void);
void stm_flash_unlock(void);
void stm_flash_lock(void);

#endif
