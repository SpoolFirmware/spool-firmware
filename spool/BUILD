constraint_setting(
    name = "platform",
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "spool",
    srcs = [
        "spool.c",
        "test.S",
    ],
    linkopts = ["-T $(execpath //spool/platforms:linker.ld)"],
    visibility = ["//visibility:public"],
    deps = [
        "//spool/platforms:config",
        "//spool/platforms:linker.ld",
    ],
)
