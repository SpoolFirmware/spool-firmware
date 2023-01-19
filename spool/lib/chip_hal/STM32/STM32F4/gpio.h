#pragma once
#include "drf/drf.h"
#include "chip_hal/gpio.h"
#include "manual/stm32f401.h"

#define DRF_HAL_GPIO_MODE_MODE            1 : 0
#define DRF_HAL_GPIO_MODE_MODE_INPUT      0x0
#define DRF_HAL_GPIO_MODE_MODE_OUTPUT     0x1
#define DRF_HAL_GPIO_MODE_MODE_AF         0x2
#define DRF_HAL_GPIO_MODE_MODE_ANALOG     0x3
#define DRF_HAL_GPIO_MODE_TYPE            2 : 2
#define DRF_HAL_GPIO_MODE_TYPE_PUSHPULL   0x0
#define DRF_HAL_GPIO_MODE_TYPE_OPENDRAIN  0x1
#define DRF_HAL_GPIO_MODE_SPEED           4 : 3
#define DRF_HAL_GPIO_MODE_SPEED_SLOW      0x0
#define DRF_HAL_GPIO_MODE_SPEED_MEDIUM    0x1
#define DRF_HAL_GPIO_MODE_SPEED_HIGH      0x2
#define DRF_HAL_GPIO_MODE_SPEED_VERY_HIGH 0x3
#define DRF_HAL_GPIO_MODE_PUPD            6 : 5
#define DRF_HAL_GPIO_MODE_PUPD_FLOATING   0U
#define DRF_HAL_GPIO_MODE_PUPD_PULL_UP    1U
#define DRF_HAL_GPIO_MODE_PUPD_PULL_DOWN  2U
#define DRF_HAL_GPIO_MODE_AF              10 : 7

#define GPIOA DRF_BASE(DRF_GPIOA)
#define GPIOB DRF_BASE(DRF_GPIOB)
#define GPIOC DRF_BASE(DRF_GPIOC)
#define GPIOD DRF_BASE(DRF_GPIOD)
#define GPIOE DRF_BASE(DRF_GPIOE)
#define GPIOH DRF_BASE(DRF_GPIOH)
