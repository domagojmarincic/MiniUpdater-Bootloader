#include "protocol.h"
#include "bootloader.h"
#include "usbd_cdc_if.h"
#include <string.h>
#include <stdio.h>
#include "stm32g0xx_hal.h"

static bool Flash_WriteBlock(uint32_t address, const uint8_t *data, uint32_t length);
static bool Flash_ProgramData(uint32_t address, const uint8_t *data, uint32_t length);
static bool Flash_ErasePage(uint32_t address);
static uint16_t CalculateChecksum();

static ProtocolState_t protocol_state = PROTOCOL_STATE_IDLE;
static uint16_t packet_size = 0;

static uint8_t rx_buffer[PACKET_SIZE + 4];
static uint32_t rx_buffer_filled = 0;
static uint32_t expected_bytes = 1;

static uint32_t current_flash_offset = PROTOCOL_APP_FLASH_ADDRESS;
static uint32_t last_erased_page = 0xFFFFFFFF;

void Protocol_Init(void)
{
	protocol_state = PROTOCOL_STATE_IDLE;
	packet_size = 0;
	rx_buffer_filled = 0;
	expected_bytes = 1;
	current_flash_offset = PROTOCOL_APP_FLASH_ADDRESS;
	last_erased_page = 0xFFFFFFFF;
}

static void ProcessCompleteMessage(void);

void Protocol_ProcessByte(uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
    {
        if (rx_buffer_filled < sizeof(rx_buffer))
        {
            rx_buffer[rx_buffer_filled++] = data[i];
        }

        if (rx_buffer_filled == expected_bytes)
        {
            ProcessCompleteMessage();
        }
    }
}

static void ProcessCompleteMessage(void)
{
    uint8_t cmd = rx_buffer[0];
    bool complete = true;

    switch (cmd)
    {
    case CMD_SET_PACKET_SIZE:
    	complete = SetPacketSize();
    	break;

    case CMD_START_TRANSFER:
        complete = StartTransfer();
        break;

    case CMD_DATA_BLOCK_WITH_CRC:
    	complete = CheckCRC();
        break;

    case CMD_END_TRANSFER:
        complete = EndTransfer();
        break;

    default:
        printf("Nepoznata komanda: 0x%02X\r\n", cmd);
        break;
    }
    if (complete)
	{
	  rx_buffer_filled = 0;
	  expected_bytes = 1;
	}
}

bool SetPacketSize()
{
	packet_size = PACKET_SIZE;
	return true;
}
bool StartTransfer()
{
	protocol_state = PROTOCOL_STATE_RECEIVING_DATA;
	return true;
}
bool CheckCRC()
{
	if(expected_bytes == 1)
	{
	  expected_bytes = CMD_SIZE + PACKET_SIZE + CRC_SIZE;
	  return false;
	}
	uint16_t received_crc = (uint16_t)(rx_buffer[1 + PACKET_SIZE] | (rx_buffer[1 + PACKET_SIZE + 1] << 8));
	uint16_t calculated_crc = CalculateChecksum(&rx_buffer[1], PACKET_SIZE);

	if (received_crc != calculated_crc)
	{
	  printf("CRC POGRESAN! primljen=0x%04X, izracunat=0x%04X\r\n", received_crc, calculated_crc);
	  /* NE piši u flash, NE pomiči offset - blok je oštećen */
	  return true;   /* poruka JEST kompletna, samo je odbijena */
	}

	if (Flash_WriteBlock(current_flash_offset, &rx_buffer[1], PACKET_SIZE))
	{
	  current_flash_offset += PACKET_SIZE;
	}
	else
	{
	  printf("Flash write GRESKA!\r\n");
	}

	return true;
}
bool EndTransfer()
{
	Bootloader_JumpToApplication();
	return true;
}

static bool Flash_ErasePage(uint32_t address)
{
    uint32_t current_page = (address - START_ADDRESS) / FLASH_PAGE_SIZE;

    if (current_page == last_erased_page)
    {
        return true;
    }

    FLASH_EraseInitTypeDef erase_init;
    uint32_t page_error;

    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_init.Page = current_page;
    erase_init.NbPages = 1;

    if (HAL_FLASHEx_Erase(&erase_init, &page_error) != HAL_OK)
    {
        return false;
    }

    last_erased_page = current_page;
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
            return false;
        }
    }

    return true;
}

static bool Flash_WriteBlock(uint32_t address, const uint8_t *data, uint32_t length)
{
    HAL_FLASH_Unlock();

    bool erase_ok = Flash_ErasePage(address);
    bool write_ok = false;

    if (erase_ok)
    {
        write_ok = Flash_ProgramData(address, data, length);
    }

    HAL_FLASH_Lock();

    return (erase_ok && write_ok);
}
static uint16_t CalculateChecksum(const uint8_t *data, uint32_t length)
{
    uint16_t crc = 0xFFFF;

    for (uint32_t i = 0; i < length; i++)
    {
        crc ^= (uint16_t)data[i] << 8;

        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (crc & 0x8000)
            {
                crc = (crc << 1) ^ 0x1021;
            }
            else
            {
                crc = crc << 1;
            }
        }
    }

    return crc;
}
