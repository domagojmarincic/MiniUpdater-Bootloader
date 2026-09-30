#ifndef FIRMWARE_STORE_H
#define FIRMWARE_STORE_H

#include <stdint.h>
#include <stdbool.h>

bool FirmwareStore_WriteBlock(uint32_t offset, const uint8_t *data, uint32_t length);
bool FirmwareStore_CopyToInternalFlash(uint32_t total_size);
void FirmwareStore_Reset(void);

#endif
