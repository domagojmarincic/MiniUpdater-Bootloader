#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include "main.h"

#define APPLICATION_ADDRESS   0x0800C000U

void Bootloader_CheckAndJump(void);
void Bootloader_JumpToApplication(void);

#endif
