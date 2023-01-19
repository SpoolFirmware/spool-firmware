cc_library(
    name = "inc",
    hdrs = glob([
        "*.h",
    ]),
    srcs = glob(["*.c"]),
    include_prefix = "chip_hal",
    deps = [
        "//spool/lib:lib",
    ],
    visibility = ["//spool/lib/chip_hal:__pkg__"],
)
