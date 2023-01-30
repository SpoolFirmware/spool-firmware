cc_library(
    name = "lib",
    srcs = glob(["*.c"]),
    hdrs = glob(["*.h"]),
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
    name = "error",
    hdrs = ["error.h"],
    include_prefix = ".",
    visibility = ["//visibility:public"],
)
