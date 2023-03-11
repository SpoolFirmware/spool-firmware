load("@rules_rust//rust:defs.bzl", "rust_static_library")

rust_static_library(
    name="planner",
    crate_name="planner",
    srcs=[
        "lib.rs",
    ],
    deps = [
        "@planner_crate_index//:fixed",
    ],
    edition="2021",
    visibility=["//visibility:public"],
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