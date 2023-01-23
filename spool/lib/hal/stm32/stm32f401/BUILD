cc_library(
    name = "lib",
    srcs = glob(["*.c"]),
    hdrs = glob(["*.h"]),
    include_prefix = "hal/stm32",
    visibility = ["//spool/platforms:__subpackages__"],
    deps = [
        "//spool/lib/hal/common:lib",
        "//spool/lib/common:lib",

        "//spool/lib/hardware_manual:mcu",

        "//spool/lib/hal/cortexm:lib",

        "//spool/lib/hal/stm32/gpio_v2:lib",
    ],
)
