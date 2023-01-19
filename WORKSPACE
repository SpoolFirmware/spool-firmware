workspace(name = "spool_firmware")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "bazel_skylib",
    urls = [
        "https://github.com/bazelbuild/bazel-skylib/releases/download/1.0.3/bazel-skylib-1.0.3.tar.gz",
        "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.0.3/bazel-skylib-1.0.3.tar.gz",
    ],
    sha256 = "1c531376ac7e5a180e0237938a2536de0c54d93f5c278634818e0efc952dd56c",
)

# Python
http_archive(
    name = "rules_python",
    sha256 = "48a838a6e1983e4884b26812b2c748a35ad284fd339eb8e2a6f3adf95307fbcd",
    strip_prefix = "rules_python-0.16.2",
    url = "https://github.com/bazelbuild/rules_python/archive/refs/tags/0.16.2.tar.gz",
)

load("@rules_python//python:repositories.bzl", "python_register_toolchains")

python_register_toolchains(
    name = "python3_9",
    # Available versions are listed in @rules_python//python:versions.bzl.
    # We recommend using the same version your team is already standardized on.
    python_version = "3.9",
)

load("@python3_9//:defs.bzl", "interpreter")

load("@rules_python//python:pip.bzl", "pip_parse")

# Create a central repo that knows about the dependencies needed from
# requirements_lock.txt.
pip_parse(
    name = "svdpatch_deps",
    python_interpreter_target = interpreter,
    requirements_lock = "//tools/svd2drf:requirements.txt",
)

# Load the starlark macro which will define your dependencies.
load("@svdpatch_deps//:requirements.bzl", "install_deps")
# Call it to define repos for your requirements.
install_deps()

# Bazel Embedded (ARM GCC)
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

# compile_commands.json

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