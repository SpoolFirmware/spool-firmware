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
        "FreeRTOSConfig.h",
        "configuration.h",
    ],
    include_prefix = ".",
    textual_hdrs = [
        "configuration.h",
        "FreeRTOSConfig.h",
    ],
    deps = [
        "//spool/cfg:core_config",
    ],
)

cc_library(
    name = "runtime",
    srcs = [
        "comm.c",
        "stepper.c",
        "platform.c",
        "thermal.c",
        "config.c",
        "config_private.h",
    ],
    copts = ["-Ispool"],
    deps = [
        ":config",
        "//spool/platforms/common:lib",
        "//spool/lib/thermistor:lib",
        "//lib/dbg:lib",
        "//lib/crt/stm32:stm32f1xx",
        "//lib/hal/stm32/stm32f103:lib",
        "//lib/syslib:lib",
        "//spool/lib/tmc_driver:lib",
    ],
    alwayslink = True,
)

exports_files(["linker.ld"])
