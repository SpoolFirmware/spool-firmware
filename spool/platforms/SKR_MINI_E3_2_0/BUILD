package(default_visibility = ["//visibility:public"])

constraint_value(
    name = "platform",
    constraint_setting = "//spool:platform",
)

platform(
    name = "skr_mini_e3_2_0",
    constraint_values = [
        ":platform",
        "//lib/hardware_manual:stm32f103",
        "//lib/freertos/portable:arm_cm3",
    ],
    parents = ["@bazel_embedded//platforms:cortex_m3"],
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
        "//lib/syslib:lib",
        "//lib/crt/stm32:stm32f1xx",
        "//lib/hal/stm32/stm32f103:lib",
    ],
    alwayslink = True,
)

exports_files(["linker.ld"])
