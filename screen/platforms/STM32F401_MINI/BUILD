package(default_visibility = ["//screen/platforms:__pkg__"])

constraint_value(
    name = "platform",
    constraint_setting = "//spool:platform",
)

platform(
    name = "stm32f401_mini",
    constraint_values = [
        ":platform",
        "//lib/hardware_manual:stm32f401",
    ],
    parents = ["@bazel_embedded//platforms:cortex_m4_fpu"],
)

cc_library(
    name = "config",
    hdrs = [
        "FreeRTOSConfig.h"
    ],
    textual_hdrs = [
        "FreeRTOSConfig.h",
    ],
    include_prefix="."
)

cc_library(
    name = "runtime",
    srcs = [
        "platform.c",
    ],
    deps = [
        ":config",
        "//spool/platforms/common:lib",
        "//lib/dbg:lib",
        "//lib/syslib:lib",
        "//lib/crt/stm32:stm32f4xx",
        "//lib/hal/stm32/stm32f401:lib",
    ],
    alwayslink = True,
)

exports_files(["linker.ld"])
