cc_library(
    name = "lib",
    srcs = glob(["*.c"]),
    hdrs = glob(["*.h"]),
    include_prefix = "hal/stm32",
    visibility = ["//spool/platforms:__subpackages__"],
    deps = [
        "//lib/common:lib",
        "//lib/hal/common:lib",

        "//lib/hardware_manual:manual",
        
        "//lib/hal/cortexm:lib",

        "//lib/hal/stm32/peripheral/gpio_v2:lib",
        "//lib/hal/stm32/peripheral/usart_v1:lib",
        "//lib/hal/stm32/peripheral/tim_v1:lib",
        "//lib/hal/stm32/peripheral/adc_v2:lib",
        "//lib/hal/stm32/peripheral/spi_v1:lib",
        "//lib/hal/stm32/peripheral/exti_v1:lib",
    ],
)
