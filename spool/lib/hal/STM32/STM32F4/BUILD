cc_library(
    name = "stm32f401",
    srcs = glob(["*.c"]),
    hdrs = glob(["*.h"]),
    include_prefix = "hal/stm32f401",
    visibility = ["//spool/platforms:__subpackages__"],
    deps = [
        "//spool/freertos:freertos",
        "//spool/lib/hal:common",
        "//spool/lib/drf",
        "//spool/lib:lib",
        "//spool/lib/hardware_manual:mcu",
    ],
)
