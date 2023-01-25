load("//toolchain:platforms.bzl", "cc_binary_with_platforms")

constraint_setting(
    name = "platform",
    visibility = ["//visibility:public"],
)

cc_binary_with_platforms(
    name = "spool",
    platforms = {
        # targets are called :spool_$platform, e.g. spool_stm32f401_mini
        "stm32f401_mini": "//spool/platforms/STM32F401_MINI:stm32f401_mini",
        "skr_pro_1_2": "//spool/platforms/SKR_PRO_1_2:skr_pro_1_2",
    },
    srcs = [
        "core/spool.c",
        "core/rtos_support.c",
        "core/step_schedule.c",
        "core/step_schedule.h",
        "core/step_execute.c",
        "core/step_execute.h",
        "core/magic_config.h",
    ],
    linkopts = ["-T $(execpath //spool/platforms:linker.ld)"],
    visibility = ["//visibility:public"],
    deps = [
        "//lib/syslib:lib",
        "//lib/fxp:lib",
        "//lib/freertos:freertos",
        "//spool/lib/hal/common:lib",
        "//spool/platforms:runtime",
        "//spool/platforms:linker.ld",
    ],
)

