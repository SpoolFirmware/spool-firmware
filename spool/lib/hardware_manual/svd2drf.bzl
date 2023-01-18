def generate_drf(name, device_yml, include_prefix='manual', visibility=None):
    for d in devices:
        native.genrule(
            name = name + "_" + d + "_header",
            srcs = [d + ".svd"],
            outs = [d + ".h"],
            exec_tools = ["//tools/svd2drf:svd2drf"],
            cmd = "$(location //tools/svd2drf:svd2drf) -o $@ $(location " + + ")",
            visibility = ["//visibility:private"]
        )

        native.cc_library(
            name = d,
            hdrs = [d+".h"],
            include_prefix=include_prefix,
            deps = [name + "_" + d + "_header"],
            visibility=visibility
        )
