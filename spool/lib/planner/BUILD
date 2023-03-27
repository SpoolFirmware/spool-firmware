load("@rules_rust//rust:defs.bzl", "rust_library", "rust_test", "rust_binary")

filegroup(
    name="planner_srcs",
    srcs=[
        "lib.rs",
        "platform.rs",
        "planner.rs",
        "stack_vec/mod.rs",
        "stack_vec/tests.rs",
    ]
)

rust_library(
    name="planner_std",
    crate_name="planner",
    srcs=[
        ":planner_srcs"
    ],
    deps = [
        "@planner_crate_index//:fixed",
        "@planner_crate_index//:fixed-sqrt",
        "@planner_crate_index//:arraydeque",
    ],
    crate_features=["std"],
    edition="2021",
    visibility=["//visibility:public"],
)

rust_library(
    name="planner",
    crate_name="planner",
    srcs=[
        ":planner_srcs"
    ],
    deps = [
        "@planner_crate_index//:fixed",
        "@planner_crate_index//:fixed-sqrt",
        "@planner_crate_index//:arraydeque",
    ],
    edition="2021",
    visibility=["//visibility:public"],
)

rust_binary(
    name = "driver",
    edition="2021",
    srcs=[
        "test_driver.rs",
    ],
    deps = [
        ":planner_std",
        "@planner_crate_index//:gcode",
    ],
    visibility=["//visibility:public"],
)

rust_test(
    name = "test",
    crate = ":planner",
    deps = [],
    visibility = ["//visibility:public"],
)

cc_library(
    name="lib",
    srcs=[],
    hdrs=["planner.h"],
    deps=[
        ":planner",
    ],
    visibility=["//visibility:public"],
)