filegroup(
    name="peripheral_patches",
    srcs=glob(["patches/devices/**/*.yaml","patches/peripherals/**/*.yaml"])
)

genrule(
    name = "stm32f401_patch_svd",
    srcs = ["svd/stm32f401.svd", ":peripheral_patches", "patches/devices/stm32f401.yaml"],
    outs = ["stm32f401.svd.patched"],
    exec_tools = ["//tools/svd2drf:svdpatch"],
    cmd = "$(location //tools/svd2drf:svdpatch) -o $@ $(location patches/devices/stm32f401.yaml)",
)

genrule(
    name = "stm32f401_header",
    srcs = [":stm32f401_patch_svd"],
    outs = ["mcu.h"],
    exec_tools = ["//tools/svd2drf:svd2drf"],
    cmd = "$(location //tools/svd2drf:svd2drf) -o $@ $<",
)

cc_library(
    name = "stm32f401",
    hdrs = [":stm32f401_header"],
    include_prefix = "manual",
    visibility = ["//spool/lib/hardware_manual:__pkg__"],
)