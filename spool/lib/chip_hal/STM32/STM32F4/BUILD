cc_library(
    name = "stm32f401",
    srcs = glob(["*.c"]),
    hdrs = glob(["*.h"]),
    include_prefix = "chip_hal/stm32f401",
    visibility = ["//spool/platforms:__subpackages__"],
    deps = [
        "//spool/lib/chip_hal:inc",
        "//spool/lib/drf",
        "//spool/lib/hardware_manual/STM32/STM32F4:stm32f401",
    ],
)
