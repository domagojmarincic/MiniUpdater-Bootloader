#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include "main.h"

#define APPLICATION_ADDRESS   0x08008000U

void Bootloader_CheckAndJump(void);
void Bootloader_JumpToApplication(void);

#endif
