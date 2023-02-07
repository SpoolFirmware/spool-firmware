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
        "//lib/common:bitops",
        "//lib/munit:lib"
    ],
    copts = [
        "-Wconversion",
        "-Warith-conversion",
    ],
)

cc_binary(
    name = "parser_test",
    srcs = [
        "core/gcode_parser_test.c",
        "core/gcode_parser.c",
        "core/gcode_parser.h",
    ],
    deps = [
        "//lib/fxp:lib",
        "//lib/common:error_std",
    ],
)

test_suite(
    name = "tests",
    tags = [],
)

cc_binary_with_platforms(
    name = "spool",
    srcs = glob([
        "core/magic_config.h",
        "core/rtos_support.c",
        "core/spool.c",
        "core/step_execute.c",
        "core/step_execute.h",
        "core/step_schedule.c",
        "core/step_schedule.h",
        "core/step_plan.c",
        "core/step_plan.h",
        "core/gcode_serial.c",
        "core/gcode_serial.h",
        "core/gcode_parser.c",
        "core/gcode_parser.h",
        "thermal/**/*.h",
        "thermal/thermal.c",
        "thermal/thermistor/*.c",
    ]),
    linkopts = [
        "-T $(execpath //spool/platforms:linker.ld)",
        "-lm",
    ],
    platforms = {
        # targets are called :spool_$platform, e.g. spool_stm32f401_mini
        # "stm32f401_mini": "//spool/platforms/STM32F401_MINI:stm32f401_mini",
        "skr_pro_1_2": "//spool/platforms/SKR_PRO_1_2:skr_pro_1_2",
        "skr_mini_e3_2_0": "//spool/platforms/SKR_MINI_E3_2_0:skr_mini_e3_2_0",
    },
    visibility = ["//visibility:public"],
    deps = [
        "//lib/freertos:freertos",
        "//lib/syslib:lib",
        "//lib/dbg:lib",
        "//lib/fxp:lib",
        "//lib/hal/common:lib",
        "//spool/platforms:linker.ld",
        "//spool/platforms:runtime",
    ],
)
