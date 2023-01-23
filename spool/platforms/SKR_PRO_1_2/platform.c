#include "FreeRTOS.h"
#include "platform/platform.h"
#include "hal/hal.h"
#include "hal/stm32/hal.h"
#include "bitops.h"

const static struct IOLine statusLED = { .group = GPIOA, .pin = 7 };

#define NR_STEPPERS 2

const static struct IOLine step[NR_STEPPERS] = {
    /* X */
    { .group = GPIOE, .pin = 9 },
    /* Y */
    { .group = GPIOE, .pin = 11 },
};

const static struct IOLine dir[NR_STEPPERS] = {
    /* X */
    { .group = GPIOF, .pin = 1 },
    /* Y */
    { .group = GPIOE, .pin = 8 },
};

const static struct IOLine en[NR_STEPPERS] = {
    /* X */
    { .group = GPIOF, .pin = 2 },
    /* Y */
    { .group = GPIOD, .pin = 7 },
};

static void setupTimer(){

void enableStepper(uint8_t stepperMask)
{
    for (uint8_t i = 0; i < NR_STEPPERS; ++i) {
        if (stepperMask & BIT(i)) {
            halGpioClear(en[i]);
        } else {
            halGpioSet(en[i]);
        }
    }
}
void setStepper(uint8_t stepperMask, uint8_t dirMask, uint8_t stepMask)
{
    for (uint8_t i = 0; i < NR_STEPPERS; ++i) {
        if (stepperMask & BIT(i)) {
            if (dirMask & BIT(i)) {
                halGpioSet(dir[i]);
            } else {
                halGpioClear(dir[i]);
            }
            if (stepMask & BIT(i)) {
                halGpioClear(step[i]);
            } else {
                halGpioSet(step[i]);
            }
        }
    }

void VectorB0() {
    portDISABLE_INTERRUPTS();
    portENABLE_INTERRUPTS();
}

void platformInit(struct PlatformConfig *config)
{
    struct HalClockConfig halClockConfig = {
        .hseFreqHz = 8000000,
        .q = 7,
        .p = 2,
        .n = 168,
        .m = 4,

        .apb2Prescaler = 2,
        .apb1Prescaler = 4,
        .ahbPrescaler = 1,
    };

    halClockInit(&halClockConfig);

    // Enable AHB Peripherials
    uint32_t ahb1enr = REG_RD32(DRF_REG(_RCC, _AHB1ENR));
    ahb1enr = FLD_SET_DRF(_RCC, _AHB1ENR, _GPIOAEN, _ENABLED, ahb1enr);
    ahb1enr = FLD_SET_DRF(_RCC, _AHB1ENR, _GPIOFEN, _ENABLED, ahb1enr);
    ahb1enr = FLD_SET_DRF(_RCC, _AHB1ENR, _GPIOEEN, _ENABLED, ahb1enr);
    REG_WR32(DRF_REG(_RCC, _AHB1ENR), ahb1enr);

    // Enable APB1 Peripherials
    uint32_t apb1enr = REG_RD32(DRF_REG(_RCC, _APB1ENR));
    apb1enr = FLD_SET_DRF(_RCC, _APB1ENR, _TIM2EN, _ENABLED, apb1enr);
    REG_WR32(DRF_REG(_RCC, _APB1ENR), apb1enr);

    halGpioSetMode(statusLED, DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT) |
                                  DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                                  DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _HIGH));
    halGpioSetMode(xStep, DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT) |
                              DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                              DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _VERY_HIGH));
    halGpioSetMode(xDir, DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT) |
                             DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                             DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _VERY_HIGH));
    halGpioSetMode(xEn, DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT) |
                            DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                            DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _VERY_HIGH));

    halGpioClear(xEn);
    halGpioClear(xDir);
}

void stepX(void)
{
    halGpioSet(xStep);
    asm(".rept 400 ; nop ; .endr");
    halGpioClear(xStep);
}

void platformMotorStep(uint8_t motor_mask, uint8_t dir_mask)
{

}

__attribute__((always_inline)) 
inline struct IOLine platformGetStatusLED(void)
{
    return statusLED;
}
