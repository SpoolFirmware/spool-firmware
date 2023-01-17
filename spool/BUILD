load("//toolchain:platforms.bzl", "cc_binary_with_platforms")

constraint_setting(
    name = "platform",
    visibility = ["//visibility:public"],
)

cc_binary_with_platforms(
    name = "spool",
    platforms = {
        # targets are called :spool_$platform, e.g. spool_stm32f401_mini
        "stm32f401_mini": "//spool/platforms/STM32F401_MINI:stm32f401_mini",
        # "skr_pro_1_2": "//spool/platforms/SKR_PRO_1_2:skr_pro_1_2",
    },
    srcs = [
        "spool.c",
    ],
    linkopts = ["-T $(execpath //spool/platforms:linker.ld)"],
    visibility = ["//visibility:public"],
    deps = [
        "//spool/platforms:config",
        "//spool/platforms:linker.ld",
    ],
)
