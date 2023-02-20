cc_library(
    name = "lib",
    srcs = [
        "clock.c",
        "adc.c",
        "adc_private.h",
    ],
    hdrs = [
        "hal.h",
        "adc.h",
        "clock.h",
    ],
    include_prefix = "hal/stm32",
    visibility = ["//spool/platforms:__subpackages__"],
    deps = [
        "//lib/common:lib",
        "//lib/hal/common:lib",

        "//lib/hardware_manual:manual",

        "//lib/hal/cortexm:lib",

        "//lib/hal/stm32/peripheral/gpio_v1:lib",
        "//lib/hal/stm32/peripheral/usart_v1:lib",
        "//lib/hal/stm32/peripheral/tim_v1:lib",
    ],
)
