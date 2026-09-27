#include "protocol.h"
#include "bootloader.h"
#include "firmware_store.h"
#include "usbd_cdc_if.h"
#include <string.h>
#include <stdio.h>
#include "stm32g0xx_hal.h"

static void ProcessCompleteMessage(void);
static bool SetPacketSize();
static bool StartTransfer();
static bool HandleDataBlock();
static bool EndTransfer();
static uint16_t CalculateChecksum(const uint8_t *data, uint32_t length);
static void Send_ACK(void);
static void Send_NACK(void);

static uint16_t packet_size = 0;
static uint8_t rx_buffer[CMD_SIZE + PACKET_SIZE + CRC_SIZE];
static uint32_t rx_buffer_filled = 0;
static uint32_t expected_bytes = 1;
static uint32_t current_flash_offset = 0;

void Protocol_Init(void)
{
	packet_size = 0;
	rx_buffer_filled = 0;
	expected_bytes = 1;
	current_flash_offset = 0;
}

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
    	complete = HandleDataBlock();
        break;

    case CMD_END_TRANSFER:
        complete = EndTransfer();
        break;

    default:
        printf("Unknown command: 0x%02X\r\n", cmd);
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
	printf("Packet size set");
	Send_ACK();

	return true;
}
bool StartTransfer()
{
	Send_ACK();
	return true;
}
bool HandleDataBlock()
{
	if(expected_bytes == 1)
	{
	  expected_bytes = CMD_SIZE + PACKET_SIZE + CRC_SIZE;
	  return false;
	}
	uint16_t received_crc = (uint16_t)(rx_buffer[PACKET_SIZE + 1] | (rx_buffer[PACKET_SIZE + 2] << 8));
	uint16_t calculated_crc = CalculateChecksum(&rx_buffer[1], PACKET_SIZE);

	if (received_crc != calculated_crc)
	{
		Send_NACK();
		return true;
	}

	if (FirmwareStore_WriteBlock(current_flash_offset, &rx_buffer[1], PACKET_SIZE))
	{
	  current_flash_offset += PACKET_SIZE;
	  Send_ACK();
	}
	else
	{
		printf("Flash write error\r\n", (unsigned long)current_flash_offset);
		Send_NACK();
	}

	return true;
}
bool EndTransfer()
{
	printf("Kopiram IZ vanjskog U unutarnji flash, ukupno %lu bajtova...\r\n",
			(unsigned long)current_flash_offset);

	if (!FirmwareStore_CopyToInternalFlash(current_flash_offset))
	{
		printf("Kopiranje GRESKA!\r\n");
		return true;
	}

	printf("Kopiranje uspjesno, skacem na aplikaciju\r\n");
    Bootloader_JumpToApplication();
	return true;
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
static void Send_ACK(void)
{
    uint8_t ack = CMD_ACK;
    CDC_Transmit_FS(&ack, 1);
}

static void Send_NACK(void)
{
    uint8_t nack = CMD_NACK;
    CDC_Transmit_FS(&nack, 1);
}
