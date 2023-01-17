package(default_visibility = ["//visibility:public"])

constraint_value(
    name = "platform",
    constraint_setting = "//spool:platform",
)

platform(
    name = "stm32f401_mini",
    constraint_values = ["platform"],
    parents = ["@bazel_embedded//platforms:cortex_m4"],
)

cc_library(
    name = "spool_config",
    srcs = [
        "startup/crt0.s",
        "startup/vectors.s",
    ],
    deps = ["//spool/lib/chip_hal/STM32/STM32F4:stm32f401"],
    hdrs = ["spool_config.h"],
    include_prefix = ".",
)

exports_files(["linker.ld"])
