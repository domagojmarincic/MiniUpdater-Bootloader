#include "bootloader.h"
#include <stdio.h>

#include "usbd_def.h"
#include "usbd_core.h"

extern PCD_HandleTypeDef hpcd_USB_DRD_FS;
extern USBD_HandleTypeDef hUsbDeviceFS;

void Bootloader_JumpToApplication(void)
{
    uint32_t app_stack_pointer = *((__IO uint32_t*)APPLICATION_ADDRESS);
    uint32_t app_reset_handler = *((__IO uint32_t*)(APPLICATION_ADDRESS + 4));

    if (app_stack_pointer == 0xFFFFFFFF)
    {
	  printf("No valid application on 0x%08lX, staying in bootloader\r\n", (unsigned long)APPLICATION_ADDRESS);
	  return;
    }

    printf("Skacem na aplikaciju: SP=0x%08lX, PC=0x%08lX\r\n",
           (unsigned long)app_stack_pointer, (unsigned long)app_reset_handler);

    /*deinicializiraj usb prije skoka*/
	USBD_DeInit(&hUsbDeviceFS);
	HAL_PCD_DeInit(&hpcd_USB_DRD_FS);

	HAL_Delay(500);

    HAL_RCC_DeInit();
    HAL_DeInit();

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    __disable_irq();

    SCB->VTOR = APPLICATION_ADDRESS;
    __set_MSP(app_stack_pointer);

    void (*app_reset_handler_func)(void) = (void (*)(void))app_reset_handler;
    app_reset_handler_func();

    while(1);
}

void Bootloader_CheckAndJump(void)
{
    if (HAL_GPIO_ReadPin(SW_1_GPIO_Port, SW_1_Pin) == GPIO_PIN_RESET)
    {
      printf("Entering bootloader\r\n");
    }
    else
    {
	  printf("Entering application(via reset)\r\n");
	  Bootloader_JumpToApplication();
    }
}
