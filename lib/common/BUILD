cc_library(
    name = "lib",
    srcs = ["error.c"],
    hdrs = [
        "bitops.h",
        "compiler.h",
        "drf.h",
        "error.h",
	"misc.h",
        "number.h",
    ],
    include_prefix = ".",
    visibility = ["//visibility:public"],
    deps = [
        "//lib/dbg:lib",
    ],
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

cc_library(
    name = "compiler",
    hdrs = ["compiler.h"],
    include_prefix = ".",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "number",
    hdrs = ["number.h"],
    include_prefix = ".",
    visibility = ["//visibility:public"],
)
