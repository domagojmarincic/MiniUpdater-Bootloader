#include <string.h>
#include <stdio.h>
#include "stm32g0xx_hal.h"
#include "firmware_store.h"
#include "spif.h"

extern spif_handle_t spif;

#define EXTERNAL_STAGING_ADDRESS   0x00000000U
#define APP_ADDRESS        0x0800C000U
#define START_ADDRESS      0x08000000U

static bool Flash_ErasePage(uint32_t address, uint32_t *last_erased_page);
static bool Flash_ProgramData(uint32_t address, const uint8_t *data, uint32_t length);

static uint32_t last_erased_external_sector = 0xFFFFFFFF;

bool FirmwareStore_WriteBlock(uint32_t offset, const uint8_t *data, uint32_t length)
{
    uint32_t address = EXTERNAL_STAGING_ADDRESS + offset;
    uint32_t sector = address / SPIF_SECTOR_SIZE;

    if (sector != last_erased_external_sector)
    {
        if (!spif_erase_sector(&spif, sector))
        {
            printf("Erase error, sector=%lu\r\n", (unsigned long)sector);
            return false;
        }
        last_erased_external_sector = sector;
    }

    uint32_t offset_in_sector = address % SPIF_SECTOR_SIZE;

    if (!spif_write_sector(&spif, sector, data, length, offset_in_sector))
    {
        printf("Write error, sector=%lu\r\n", (unsigned long)sector);
        return false;
    }

    return true;
}

bool FirmwareStore_CopyToInternalFlash(uint32_t total_size)
{
    static uint8_t buffer[256];
    uint32_t copied = 0;
    uint32_t last_erased_page = 0xFFFFFFFF;

    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPERR);
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_PROGERR);
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_WRPERR);
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_PGAERR);
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_SIZERR);
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_PGSERR);
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_MISERR);
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_FASTERR);
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);

    while (copied < total_size)
    {
        uint32_t chunk = (total_size - copied < sizeof(buffer)) ? (total_size - copied) : sizeof(buffer);

        if (!spif_read_address(&spif, EXTERNAL_STAGING_ADDRESS + copied, buffer, chunk))
        {
            printf("Read error, offset=%lu\r\n", (unsigned long)copied);
            HAL_FLASH_Lock();
            return false;
        }

        uint32_t internal_address = APP_ADDRESS + copied;

        if (!Flash_ErasePage(internal_address, &last_erased_page))
        {
            HAL_FLASH_Lock();
            return false;
        }

        if (!Flash_ProgramData(internal_address, buffer, chunk))
        {
            HAL_FLASH_Lock();
            return false;
        }

        copied += chunk;
    }

    HAL_FLASH_Lock();
    return true;
}

void FirmwareStore_Reset(void)
{
    last_erased_external_sector = 0xFFFFFFFF;
}

static bool Flash_ErasePage(uint32_t address, uint32_t *last_erased_page)
{
    uint32_t page = (address - START_ADDRESS) / FLASH_PAGE_SIZE;

    if (page == *last_erased_page)
    {
        return true;
    }

    FLASH_EraseInitTypeDef erase_init;
    uint32_t page_error;

    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_init.Page = page;
    erase_init.NbPages = 1;
    erase_init.Banks = FLASH_BANK_1;

    if (HAL_FLASHEx_Erase(&erase_init, &page_error) != HAL_OK)
    {
        printf("Erase error");
        return false;
    }

    *last_erased_page = page;
    return true;
}

static bool Flash_ProgramData(uint32_t address, const uint8_t *data, uint32_t length)
{
    for (uint32_t i = 0; i < length; i += 8)
    {
        uint64_t doubleword;
        memcpy(&doubleword, &data[i], 8);

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address + i, doubleword) != HAL_OK)
        {
            printf("Write errot\r\n");
            return false;
        }
    }
    return true;
}
