package(default_visibility = ["//visibility:public"])

constraint_value(
    name = "platform",
    constraint_setting = "//spool:platform",
)

platform(
    name = "stm32f401_mini",
    constraint_values = ["platform"],
    parents = ["@bazel_embedded//platforms:cortex_m4_fpu"],
)

cc_library(
    name = "spool_config",
    srcs = [
        "platform.c",
        "startup/crt0.s",
        "startup/vectors.s",
    ],
    hdrs = [
        "FreeRTOSConfig.h",
        "spool_config.h",
    ],
    include_prefix = ".",
    textual_hdrs = [
        "spool_config.h",
        "FreeRTOSConfig.h",
    ],
    deps = [
        "//spool/lib/chip_hal/STM32/STM32F4:stm32f401",
        "//spool/platforms:lib",
    ],
    alwayslink = True,
)

exports_files(["linker.ld"])
