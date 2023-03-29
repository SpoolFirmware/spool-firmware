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
        "//lib/lvgl/config:12864",
    ],
    parents = ["@bazel_embedded//platforms:cortex_m4_fpu"],
)

cc_library(
    name = "config",
    hdrs = [
        "configuration.h",
        "FreeRTOSConfig.h",
    ],
    textual_hdrs = [
        "configuration.h",
        "FreeRTOSConfig.h",
    ],
    deps = [
        "//spool/cfg:core_config",
    ],
    include_prefix="."
)

cc_library(
    name = "runtime",
    srcs = [
        "platform_private.h",
        "platform.c",
        "thermal.c",
        "communication.c",
        "config.c",
    ],
    copts = ["-Ispool"],
    deps = [
        ":config",
        "//spool/platforms/common:lib",
        "//spool/lib/thermistor:lib",
        "//spool/lib/st7920:lib",
        "//spool/lib/encoder:lib",
        "//spool/lib/ui/st7920:lib",

        "//lib/syslib:lib",
        "//lib/dbg:lib",
        "//lib/crt/stm32:stm32f4xx",
        "//lib/hal/stm32/stm32f407:lib",
    ],
    alwayslink = True,
)

exports_files(["linker.ld"])
