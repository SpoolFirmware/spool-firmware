package(default_visibility = ["//spool/platforms:__pkg__"])

constraint_value(
    name = "platform",
    constraint_setting = "//spool:platform",
)

platform(
    name = "stm32f401_mini",
    constraint_values = [
        ":platform",
        "//spool/platforms/mcu:stm32f401",
    ],
    parents = ["@bazel_embedded//platforms:cortex_m4_fpu"],
)

cc_library(
    name = "config",
    hdrs = [
        "spool_config.h", 
        "FreeRTOSConfig.h"
    ],
    textual_hdrs = [
        "spool_config.h",
        "FreeRTOSConfig.h",
    ],
    include_prefix="."
)

cc_library(
    name = "runtime",
    srcs = [
        "platform.c",
        "startup/crt0.s",
        "startup/vectors.s",
    ],
    deps = [
        ":config",
        "//spool/lib/hal/STM32/STM32F4:stm32f401",
        "//spool/platforms:lib",
    ],
    alwayslink = True,
)

exports_files(["linker.ld"])
