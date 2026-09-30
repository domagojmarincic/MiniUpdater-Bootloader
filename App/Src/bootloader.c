#include "bootloader.h"
#include <stdio.h>

#include "usbd_def.h"
#include "usbd_core.h"

extern PCD_HandleTypeDef hpcd_USB_DRD_FS;
extern USBD_HandleTypeDef hUsbDeviceFS;
extern UART_HandleTypeDef huart2;

void Bootloader_JumpToApplication(void)
{
    uint32_t app_stack_pointer = *((__IO uint32_t*)APPLICATION_ADDRESS);
    uint32_t app_reset_handler = *((__IO uint32_t*)(APPLICATION_ADDRESS + 4));

    if (app_stack_pointer == 0xFFFFFFFF)
    {
	  printf("No valid application on 0x%08lX\r\n", (unsigned long)APPLICATION_ADDRESS);
	  return;
    }

    printf("Jumping on application\r\n");

	USBD_DeInit(&hUsbDeviceFS);
	HAL_PCD_DeInit(&hpcd_USB_DRD_FS);
    HAL_UART_DeInit(&huart2);

	HAL_Delay(500);

    HAL_RCC_DeInit();
    HAL_DeInit();

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    __disable_irq();

    SCB->VTOR = APPLICATION_ADDRESS;
    __set_MSP(app_stack_pointer);

    __enable_irq();

    void (*app_reset_handler_func)(void) = (void (*)(void))app_reset_handler;
    app_reset_handler_func();

}

void Bootloader_CheckAndJump(void)
{
    if (HAL_GPIO_ReadPin(SW_1_GPIO_Port, SW_1_Pin) == GPIO_PIN_RESET)
    {
        printf("Entering bootloader\r\n");
    }
    else
    {
        printf("Entering application\r\n");
        Bootloader_JumpToApplication();
    }
}
