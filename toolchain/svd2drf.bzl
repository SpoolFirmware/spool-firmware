def generate_drf(name, devices):
    for d in devices:
        native.genrule(
            name = name + "_" + d,
            srcs = [d + ".svd"],
            outs = [d + ".h"],
            tools = ["//tools/svd2drf:svd2drf"],
            cmd = "$(location //tools/svd2drf:svd2drf) -o $@ $<",
        )
