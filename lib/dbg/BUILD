cc_library(
    name = "lib",
    srcs = glob(["*.c"]),
    hdrs = glob(["*.h"]),
    include_prefix = ".",
    visibility = ["//visibility:public"],
    deps = [
        "//lib/common:lib",
        "//lib/freertos:freertos",
    ],
)
