#pragma once
#include "bitops.h"
#include "hal/gpio.h"

enum GPIOGroupEnable {
    EnableGPIOA = BIT(0),
    EnableGPIOB = BIT(1),
    EnableGPIOC = BIT(2),
    EnableGPIOD = BIT(3),
    EnableGPIOE = BIT(4),
    EnableGPIOF = BIT(5),
    EnableGPIOG = BIT(6),
};

#define GPIO__COUNT     7U

struct HalGPIOConfig {
    uint32_t groupEnable; //! @ref GPIOGroupEnable
};

#define DRF_HAL_GPIO_MODE_MODE                1 : 0
#define DRF_HAL_GPIO_MODE_MODE_INPUT          0x0
#define DRF_HAL_GPIO_MODE_MODE_OUTPUT10       0x1 // 10 Mhz
#define DRF_HAL_GPIO_MODE_MODE_OUTPUT2        0x2 // 2 Mhz
#define DRF_HAL_GPIO_MODE_MODE_OUTPUT50       0x3 // 50 Mhz
#define DRF_HAL_GPIO_MODE_TYPE                3 : 2
#define DRF_HAL_GPIO_MODE_TYPE_PUSH_PULL      0U
#define DRF_HAL_GPIO_MODE_TYPE_OPEN_DRAIN     1U
#define DRF_HAL_GPIO_MODE_TYPE_ALT_PUSH_PULL  2U
#define DRF_HAL_GPIO_MODE_TYPE_ALT_OPEN_DRAIN 3U
#define DRF_HAL_GPIO_MODE_TYPE_ANALOG         0U
#define DRF_HAL_GPIO_MODE_TYPE_FLOATING       1U
#define DRF_HAL_GPIO_MODE_TYPE_PULL_UP_DN     2U
