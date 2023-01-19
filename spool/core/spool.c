//
// Created by codetector on 1/14/23.
//
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#include "platform/platform.h"
#include "chip_hal/hal.h"
#include "chip_hal/stm32f401/gpio.h"
#include "spool_config.h"

static portTASK_FUNCTION_PROTO( vLEDFlashTask1, pvParameters );
static portTASK_FUNCTION( vLEDFlashTask1, pvParameters )
{
    ( void ) pvParameters;
    struct IOLine led1 = chipHalGpioLineConstruct(GPIOC, 13);
    chipHalGpioSetMode(led1, 
        DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT)
        | DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL)
        | DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _VERY_HIGH)
        );
    
    for( ; ; )
    {
        chipHalGpioSet(led1);
        vTaskDelay(25);
        chipHalGpioClear(led1);
        vTaskDelay(25);
    }
}

static portTASK_FUNCTION_PROTO( vLEDFlashTask2, pvParameters );
static portTASK_FUNCTION( vLEDFlashTask2, pvParameters )
{
    ( void ) pvParameters;
    struct IOLine led2 = chipHalGpioLineConstruct(GPIOA, 11);
    chipHalGpioSetMode(led2, 
        DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT)
        | DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL)
        | DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _VERY_HIGH)
        );
    
    for( ; ; )
    {
        chipHalGpioClear(led2);
        vTaskDelay(50);
        chipHalGpioSet(led2);
        vTaskDelay(50);
    }
}

void main()
{
    struct PlatformConfig platformConfig = { 0 };
    platformInit(&platformConfig);

    uint32_t rcc = REG_RD32(DRF_REG(_RCC,_AHB1ENR));
    rcc = FLD_SET_DRF(_RCC,_AHB1ENR,_GPIOAEN, _ENABLED, rcc);
    rcc = FLD_SET_DRF(_RCC,_AHB1ENR,_GPIOBEN, _ENABLED, rcc);
    rcc = FLD_SET_DRF(_RCC,_AHB1ENR,_GPIOCEN, _ENABLED, rcc);
    REG_WR32(DRF_REG(_RCC,_AHB1ENR), rcc);

    xTaskCreate( vLEDFlashTask1, "LEDx", 1024, NULL, tskIDLE_PRIORITY+1, ( TaskHandle_t * ) NULL );
    xTaskCreate( vLEDFlashTask2, "LEDx", 1024, NULL, tskIDLE_PRIORITY+1, ( TaskHandle_t * ) NULL );

    vTaskStartScheduler();
    for(;;){}
}