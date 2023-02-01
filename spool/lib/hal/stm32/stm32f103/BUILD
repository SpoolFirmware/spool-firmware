cc_library(
    name = "lib",
    srcs = glob(["*.c"]),
    hdrs = glob(["*.h"]),
    include_prefix = "hal/stm32",
    visibility = ["//spool/platforms:__subpackages__"],
    deps = [
        "//lib/common:lib",
        "//spool/lib/hal/common:lib",

        "//spool/lib/hardware_manual:manual",

        "//spool/lib/hal/cortexm:lib",
    ],
)
