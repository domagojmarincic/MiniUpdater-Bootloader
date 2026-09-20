#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

#define CMD_SET_PACKET_SIZE      0x01U
#define CMD_START_TRANSFER       0x02U
#define CMD_DATA_BLOCK_WITH_CRC   0x03U
#define CMD_END_TRANSFER         0x04U
#define CMD_ACK                  0x05U
#define CMD_NACK                 0x06U

#define PACKET_SIZE              256U
#define CMD_SIZE				 1U
#define CRC_SIZE				 2U

#define START_ADDRESS                0X08000000U
#define PROTOCOL_APP_FLASH_ADDRESS   0x08008000U

typedef enum {
    PROTOCOL_STATE_IDLE = 0,
    PROTOCOL_STATE_WAIT_PACKET_SIZE,
    PROTOCOL_STATE_RECEIVING_DATA,
} ProtocolState_t;

void Protocol_Init(void);
void Protocol_ProcessByte(uint8_t *data, uint32_t len);
bool SetPacketSize();
bool StartTransfer();
bool CheckCRC();
bool EndTransfer();

#endif
