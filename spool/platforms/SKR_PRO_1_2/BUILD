package(default_visibility = ["//visibility:public"])

constraint_value(
    name = "platform",
    constraint_setting = "//spool:platform",
)

platform(
    name = "skr_pro_1_2",
    constraint_values = [
        ":platform",
        "//spool/lib/hardware_manual:stm32f407",
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
        "platform_private.h",
        "platform.c",
        "communication.c",
    ],
    deps = [
        ":config",
        "//lib/crt/stm32:stm32f4xx",
        "//spool/platforms/common:lib",
        "//lib/syslib:lib",
        "//spool/lib/hal/stm32/stm32f407:lib",
    ],
    alwayslink = True,
)

exports_files(["linker.ld"])
