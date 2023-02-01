package(default_visibility = ["//visibility:public"])

constraint_value(
    name = "platform",
    constraint_setting = "//spool:platform",
)

platform(
    name = "skr_pro_1_2",
    constraint_values = [
        ":platform",
        "//lib/hardware_manual:stm32f407",
        "//lib/freertos/portable:arm_cm4f",
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
        "platform_private.h",
        "platform.c",
        "communication.c",
    ],
    deps = [
        ":config",
        "//spool/platforms/common:lib",
        "//lib/syslib:lib",
        "//lib/dbg:lib",
        "//lib/crt/stm32:stm32f4xx",
        "//lib/hal/stm32/stm32f407:lib",
    ],
    alwayslink = True,
)

exports_files(["linker.ld"])
