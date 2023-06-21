load("//toolchain:platforms.bzl", "cc_binary_with_platforms")

constraint_setting(
    name = "platform",
    visibility = ["//visibility:public"],
)

cc_test(
    name = "motion_test",
    size = "small",
    srcs = [
        "gcode/gcode.h",
        "gcode/gcode_parser.c",
        "gcode/gcode_parser.h",
        "motion/motion.c",
        "motion/motion.h",
        "motion/step_plan_ng.c",
        "motion/step_plan_ng.h",
        "test/motion/motion_test.c",
    ],
    copts = [
        "-Wconversion",
        "-Warith-conversion",
        "-Ispool",
        "-Ispool/test",
        "-Ispool/test/config",
    ],
    deps = [
        "//lib/fxp:lib",
        "//lib/munit:lib",
        "//lib/test_compat:lib",
    ],
)

cc_test(
    name = "step_plan_ng_test",
    size = "small",
    srcs = [
        "motion/step_plan_ng.c",
        "motion/step_plan_ng.h",
        "test/motion/step_plan_ng_test.c",
    ],
    copts = [
        "-Werror=conversion",
        "-Werror=arith-conversion",
        "-Wpointer-sign",
        "-Ispool",
        "-Ispool/test",
        "-Ispool/test/config",
    ],
    deps = [
        "//lib/fxp:lib",
        "//lib/munit:lib",
        "//lib/test_compat:lib",
    ],
)

cc_binary(
    name = "parser_test",
    srcs = [
        "gcode/gcode_parser.c",
        "gcode/gcode_parser.h",
        "gcode/gcode_parser_test.c",
    ],
    copts = ["-Ispool"],
    deps = [
        "//lib/fxp:lib",
        "//lib/test_compat:lib",
    ],
)

test_suite(
    name = "tests",
    tags = [],
)

cc_binary_with_platforms(
    name = "spool",
    srcs = glob([
        "ui/*.c",
        "ui/*.h",
        "thermal/*.h",
        "thermal/*.c",
        "motion/*.h",
        "motion/*.c",
        "gcode/*.h",
        "core/*.h",
        "core/*.c",
    ]) + [
        "gcode/gcode_serial.c",
        "gcode/gcode_parser.c",
    ],
    copts = ["-Ispool"],
    linkopts = [
        "-T $(execpath //spool/platforms:linker.ld)",
        "-lm",
        "-z noexecstack",
        "-Wl,--allow-multiple-definition", # SUS option but LOL 
    ],
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
        "//lib/lvgl:lib",
        "//spool/lib/bltouch:lib",
        "//spool/lib/encoder:lib",
        "//spool/lib/planner:lib",
        "//spool/lib/sdspi:lib",
        "//spool/lib/thermistor:lib",
        "//spool/platforms:linker.ld",
        "//spool/platforms:runtime",
    ],
)
