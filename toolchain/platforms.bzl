load("//toolchain:copy_file.bzl", "copy_cmd", "copy_bash")
load("@rules_cc//cc:defs.bzl", "cc_binary")

def _transition_impl(settings, attr):
    # Attaching the special prefix "//comand_line_option" to the name of a native
    # flag makes the flag available to transition on. The result of this transition
    # is to set --platforms
    if str(settings["//command_line_option:compilation_mode"]) == 'dbg':
        build_type = "dbg"
    else:
        build_type = "opt"

    return {
        "//command_line_option:platforms": attr.platform,
        "//command_line_option:compilation_mode": build_type,
    }

platform_transition = transition(
    implementation = _transition_impl,
    inputs = ["//command_line_option:compilation_mode"],
    # We declare which flags the transition will be writing. The returned dict(s)
    # of flags must have keyset(s) that contains exactly this list.
    outputs = ["//command_line_option:platforms", "//command_line_option:compilation_mode"],
)

def _impl(ctx):
    if ctx.attr.is_windows:
        copy = copy_cmd
    else:
        copy = copy_bash

    output_files = []
    copy_pairs = []
    for f in ctx.files.srcs:
        src_name = f.path.split("/")[-1]
        output_file = ctx.actions.declare_file(ctx.label.name + "/" + src_name)
        output_files.append(output_file)
        copy_pairs.append((f, output_file))
    copy(ctx, copy_pairs)

    runfiles = ctx.runfiles(files = output_files)

    return DefaultInfo(files = depset(output_files), data_runfiles = runfiles)

# Define a rule that uses the transition.
_with_platform = rule(
    implementation = _impl,
    # Attach the transition to the rule using the `cfg` attribute. This will transition
    # the configuration of this target, which the target's descendents will inherit.
    attrs = {
        "srcs": attr.label_list(
            cfg = platform_transition,
            allow_files = True,
            mandatory = True,
        ),
        # This attribute is required to use starlark transitions. It allows
        # allowlisting usage of this rule. For more information, see
        # https://docs.bazel.build/versions/master/skylark/config.html#user-defined-transitions
        "_allowlist_function_transition": attr.label(
            default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
        ),
        "is_windows": attr.bool(mandatory = True),
        "platform": attr.string(default = ""),
    },
    provides = [DefaultInfo],
)

def with_platform(name, srcs, platform, **kwargs):
    _with_platform(
        name = name,
        srcs = srcs,
        platform = platform,
        is_windows = select({
            "@bazel_tools//src/conditions:host_windows": True,
            "//conditions:default": False,
        }),
        **kwargs
    )

def cc_binary_with_platforms(name, platforms, visibility=None, **kwargs):
    elf_name = name + ".elf"
    bin_name = name + ".bin"
    bin_target = name + "_bin"

    cc_binary(
        name = elf_name,
        visibility = ["//visibility:private"],
        **kwargs
    )

    native.genrule(
        name = bin_target,
        visibility = ["//visibility:private"],
        srcs = [elf_name,],
        outs = [bin_name,],
        toolchains = ["@bazel_tools//tools/cpp:current_cc_toolchain"],
        cmd = "$(OBJCOPY) -O binary $< $@",
    )

    for p, target in platforms.items():
        with_platform(
            name = p,
            srcs = [elf_name, bin_name],
            visibility = visibility,
            platform = target,
        )

    native.filegroup(
        name = name,
        srcs = platforms.keys(),
        visibility = visibility,
    )
