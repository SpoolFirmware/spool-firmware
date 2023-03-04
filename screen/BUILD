load("//toolchain:platforms.bzl", "cc_binary_with_platforms")

constraint_setting(
    name = "platform",
    visibility = ["//visibility:public"],
)

cc_binary_with_platforms(
    name = "screen",
    srcs = ["screen.c"],
    copts = ["-Iscreen"],
    linkopts = [
        "-T $(execpath //screen/platforms:linker.ld)",
        "-lm",
    ],
    platforms = {
        # targets are called :spool_$platform, e.g. spool_stm32f401_mini
        "stm32f401_mini": "//screen/platforms/STM32F401_MINI:stm32f401_mini",
    },
    visibility = ["//visibility:public"],
    deps = [
        "//lib/syslib:lib",
        "//lib/hal/common:lib",
        "//lib/lvgl:lib",
        "//spool/lib/sdspi:lib",
        "//spool/lib/thermistor:lib",
        "//spool/platforms:linker.ld",
        "//spool/platforms:runtime",
    ],
)