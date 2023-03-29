cc_library(
    name = "lib",
    hdrs = glob(["*.h"]),
    srcs = glob(["*.c"]),
    include_prefix = "hal",
    alwayslink=True,
    deps = [
        "//lib/common:lib",
        "//lib/freertos:freertos",
    ],
    visibility = ["//visibility:public"],
)
