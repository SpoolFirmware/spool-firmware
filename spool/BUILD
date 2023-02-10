load("//toolchain:platforms.bzl", "cc_binary_with_platforms")

constraint_setting(
    name = "platform",
    visibility = ["//visibility:public"],
)

cc_test(
    name = "step_plan_ng_test",
    size = "small",
    srcs = [
        "core/magic_config.h",
        "motion/step_plan_ng.c",
        "motion/step_plan_ng.h",
        "motion/step_plan_ng_test.c",
    ],
    copts = [
        "-Wconversion",
        "-Warith-conversion",
    ],
    deps = [
        "//lib/common:bitops",
        "//lib/common:compiler",
        "//lib/common:error_std",
        "//lib/common:number",
        "//lib/fxp:lib",
        "//lib/munit:lib",
    ],
)

cc_binary(
    name = "parser_test",
    srcs = [
        "gcode/gcode_parser.c",
        "gcode/gcode_parser.h",
        "gcode/gcode_parser_test.c",
    ],
    deps = [
        "//lib/common:error_std",
        "//lib/fxp:lib",
    ],
)

test_suite(
    name = "tests",
    tags = [],
)

cc_binary_with_platforms(
    name = "spool",
    srcs = glob([
        "thermal/*.h",
        "thermal/*.c",
        "motion/*.h",
        "gcode/*.h",

        "core/*.h",
    ]) + 
    [
        "core/rtos_support.c",
        "core/spool.c",
        "motion/motion.c",
        "motion/step_execute.c",
        "motion/step_schedule.c",
        "motion/step_plan_ng.c",
        "gcode/gcode_serial.c",
        "gcode/gcode_parser.c",
    ],
    linkopts = [
        "-T $(execpath //spool/platforms:linker.ld)",
        "-lm",
    ],
    stamp = 1,
    copts = ["-Ispool"],
    platforms = {
        # targets are called :spool_$platform, e.g. spool_stm32f401_mini
        # "stm32f401_mini": "//spool/platforms/STM32F401_MINI:stm32f401_mini",
        "skr_pro_1_2": "//spool/platforms/SKR_PRO_1_2:skr_pro_1_2",
        "skr_mini_e3_2_0": "//spool/platforms/SKR_MINI_E3_2_0:skr_mini_e3_2_0",
    },
    visibility = ["//visibility:public"],
    deps = [
        "//lib/dbg:lib",
        "//lib/freertos",
        "//lib/fxp:lib",
        "//lib/hal/common:lib",
        "//lib/syslib:lib",
        "//spool/lib/thermistor:lib",
        "//spool/platforms:linker.ld",
        "//spool/platforms:runtime",
    ],
)
