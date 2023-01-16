load("@bazel_skylib//rules:copy_file.bzl", "copy_file")
load("@rules_cc//cc:defs.bzl", "cc_binary")

def _transition_impl(settings, attr):
    _ignore = settings

    # Attaching the special prefix "//comand_line_option" to the name of a native
    # flag makes the flag available to transition on. The result of this transition
    # is to set --cpu
    # We read the value from the attr also named `cpu` which allows target writers
    # to modify how the transition works. This could also just be a hardcoded
    # string like "x86" if you didn't want to give target writers that power.
    return {"//command_line_option:platforms": attr.platforms}

# Define a transition.
platforms_transition = transition(
    implementation = _transition_impl,
    inputs = [],
    # We declare which flags the transition will be writing. The returned dict(s)
    # of flags must have keyset(s) that contains exactly this list.
    outputs = ["//command_line_option:platforms"],
)

def _impl(ctx):
    return DefaultInfo(files = depset(ctx.files.srcs))

# Define a rule that uses the transition.
binary_with_platform = rule(
    implementation = _impl,
    # Attach the transition to the rule using the `cfg` attribute. This will transition
    # the configuration of this target, which the target's descendents will inherit.
    attrs = {
        "srcs": attr.label_list(
            cfg = platforms_transition,
            allow_files = True,
        ),
        # This attribute is required to use starlark transitions. It allows
        # allowlisting usage of this rule. For more information, see
        # https://docs.bazel.build/versions/master/skylark/config.html#user-defined-transitions
        "_allowlist_function_transition": attr.label(
            default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
        ),
        "platforms": attr.string(default = ""),
    },
)

def cc_binary_with_platforms(name, platforms, visibility=None, **kwargs):
    for p, target in platforms.items():
        cc_binary(
            name = name + "_" + p + "_cc_binary",
            visibility = ["//visibility:private"],
            **kwargs
        )

        binary_with_platform(
            name = name + "_" + p + "_platform",
            srcs = [name + "_" + p + "_cc_binary"],
            visibility = ["//visibility:private"],
            platforms = target,
        )

        copy_file(
            name = name + "_" + p,
            src = name + "_" + p + "_platform",
            out = p + "/" + name,
            visibility = visibility,
        )

    native.filegroup(
        name = "spool",
        srcs = [name + "_" + p for p in platforms.keys()],
        visibility = visibility,
    )
