load("//toolchain:platforms.bzl", "cc_binary_with_platforms")

constraint_setting(
    name = "platform",
    visibility = ["//visibility:public"],
)

cc_test(
    name = "step_plan_test",
    size = "small",
    srcs = [
        "core/step_plan_test.c",
        "core/step_plan.c",
        "core/step_plan.h",
        "core/magic_config.h",
    ],
    deps = [
        "//lib/fxp:lib", 
        "//lib/munit:lib"
    ],
    copts = [
        "-Wconversion",
        "-Warith-conversion",
    ],

)

test_suite(
    name = "tests",
    tags = [],
)

cc_binary_with_platforms(
    name = "spool",
    srcs = [
        "core/magic_config.h",
        "core/rtos_support.c",
        "core/spool.c",
        "core/step_execute.c",
        "core/step_execute.h",
        "core/step_schedule.c",
        "core/step_schedule.h",
        "core/step_plan.c",
        "core/step_plan.h",
    ],
    copts = [
        "-Wconversion",
        "-Warith-conversion",
    ],
    linkopts = [
        "-T $(execpath //spool/platforms:linker.ld)",
        "-lm",
    ],
    platforms = {
        # targets are called :spool_$platform, e.g. spool_stm32f401_mini
        "stm32f401_mini": "//spool/platforms/STM32F401_MINI:stm32f401_mini",
        "skr_pro_1_2": "//spool/platforms/SKR_PRO_1_2:skr_pro_1_2",
    },
    visibility = ["//visibility:public"],
    deps = [
        "//lib/freertos:freertos",
        "//lib/syslib:lib",
        "//lib/dbg:lib",
        "//lib/fxp:lib",
        "//spool/lib/hal/common:lib",
        "//spool/platforms:linker.ld",
        "//spool/platforms:runtime",
    ],
)
