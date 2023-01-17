workspace(name = "spool_firmware")

local_repository(
    name = "bazel_embedded",
    path = "bazel-embedded",
)

load("@bazel_embedded//:bazel_embedded_deps.bzl", "bazel_embedded_deps")

bazel_embedded_deps()

load("@bazel_embedded//platforms:execution_platforms.bzl", "register_platforms")

register_platforms()

load(
    "@bazel_embedded//toolchains/compilers/gcc_arm_none_eabi:gcc_arm_none_repository.bzl",
    "gcc_arm_none_compiler",
)

gcc_arm_none_compiler()

load("@bazel_embedded//toolchains/gcc_arm_none_eabi:gcc_arm_none_toolchain.bzl", "register_gcc_arm_none_toolchain")

register_gcc_arm_none_toolchain()

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# Hedron's Compile Commands Extractor for Bazel
# https://github.com/hedronvision/bazel-compile-commands-extractor
# https://github.com/SpoolFirmware/bazel-compile-commands-extractor with -mfpu=auto removal
http_archive(
    name = "hedron_compile_commands",

    # Replace the commit hash in both places (below) with the latest
    url = "https://github.com/SpoolFirmware/bazel-compile-commands-extractor/archive/efefb12d54cfeedc0d6b705f2346496c69955fd5.tar.gz",
    strip_prefix = "bazel-compile-commands-extractor-efefb12d54cfeedc0d6b705f2346496c69955fd5",
    # When you first run this tool, it'll recommend a sha256 hash to put here with a message like: "DEBUG: Rule 'hedron_compile_commands' indicated that a canonical reproducible form can be obtained by modifying arguments sha256 = ..."
)
load("@hedron_compile_commands//:workspace_setup.bzl", "hedron_compile_commands_setup")
hedron_compile_commands_setup()