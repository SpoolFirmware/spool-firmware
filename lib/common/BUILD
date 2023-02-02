cc_library(
    name = "lib",
    srcs = ["error.c"],
    hdrs = ["bitops.h", "drf.h", "error.h"],
    include_prefix=".",
    deps = [
        "//lib/dbg:lib",
    ],
    visibility=["//visibility:public"]
)

cc_library(
    name = "bitops",
    hdrs = ["bitops.h"],
    include_prefix = ".",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "error_std",
    srcs = ["error_std.c"],
    hdrs = ["error.h"],
    include_prefix = ".",
    visibility = ["//visibility:public"],
)
