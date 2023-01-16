package(default_visibility = ["//visibility:public"])

constraint_value(
    name = "platform",
    constraint_setting = "//spool:platform",
)

platform(
    name = "skr_pro_1_2",
    constraint_values = ["platform"],
    parents = ["@bazel_embedded//platforms:cortex_m4"],
)

cc_library(
    name = "spool_config",
    hdrs = ["spool_config.h"],
    include_prefix = "config",
)

exports_files(["linker.ld"])
