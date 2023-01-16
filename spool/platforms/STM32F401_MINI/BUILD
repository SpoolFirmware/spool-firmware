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
    hdrs = ["spool_config.h"],
    include_prefix = ".",
)

exports_files(["linker.ld"])
