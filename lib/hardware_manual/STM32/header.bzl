load("@rules_cc//cc:defs.bzl", "cc_library")

# require target peripheral_patches
def hardware_manual_library(name):
    svd_src_file = "svd/" + name + ".svd"
    svd_patch_file = "patches/devices/" + name + ".yaml"
    svd_patched_file = name + ".svd.patched"
    svd_patched_target = name + "_patched_svd"

    svd_drf_header = name + "_drf_header"
    svd_irq_header = name + "_irq_header"

    native.genrule(
        name = svd_patched_target,
        srcs = [svd_src_file, ":peripheral_patches", svd_patch_file],
        outs = [svd_patched_file],
        exec_tools = ["//tools/svd2drf:svdpatch"],
        cmd = "$(location //tools/svd2drf:svdpatch) -o $@ $(location " + svd_patch_file + ")",
    )

    native.genrule(
        name = svd_drf_header,
        srcs = [svd_patched_target],
        outs = [name + "/mcu.h"],
        exec_tools = ["//tools/svd2drf:svd2drf"],
        cmd = "$(location //tools/svd2drf:svd2drf) --drf -o $@ $<",
    )

    native.genrule(
        name = svd_irq_header,
        srcs = [svd_patched_target],
        outs = [name + "/irq.h"],
        exec_tools = ["//tools/svd2drf:svd2drf"],
        cmd = "$(location //tools/svd2drf:svd2drf) --irq -o $@ $<",
    )

    cc_library(
        name = name,
        hdrs = [svd_drf_header, svd_irq_header],
        include_prefix = "manual",
        strip_include_prefix = name,
        visibility = ["//lib/hardware_manual:__pkg__"],
    )
