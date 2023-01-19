load("@svdpatch_deps//:requirements.bzl", "requirement")

py_library(
    name = "cmsis_svd",
    srcs = [
        "cmsis_svd/model.py",
        "cmsis_svd/parser.py",
    ],
    srcs_version = "PY3",
    visibility = ["//visibility:private"],
)

py_binary(
    name = "svd2drf",
    srcs = ["svd2drf.py"],
    main = "svd2drf.py",
    srcs_version = "PY3",
    visibility = ["//visibility:public"],
    deps = [":cmsis_svd"],
)

py_binary(
    name = "svdpatch",
    srcs = ["svdpatch.py"],
    main = "svdpatch.py",
    srcs_version = "PY3",
    visibility = ["//visibility:public"],
    deps = [
        requirement("lxml"),
        requirement("pyyaml"),
        requirement("braceexpand")
    ],
)

exports_files(["requirements.txt"])
