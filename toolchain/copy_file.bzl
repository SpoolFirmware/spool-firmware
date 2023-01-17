COPY_EXECUTION_REQUIREMENTS = {
    # ----------------+-----------------------------------------------------------------------------
    # no-remote       | Prevents the action or test from being executed remotely or cached remotely.
    #                 | This is equivalent to using both `no-remote-cache` and `no-remote-exec`.
    # ----------------+-----------------------------------------------------------------------------
    # no-cache        | Results in the action or test never being cached (remotely or locally)
    # ----------------+-----------------------------------------------------------------------------
    # See https://bazel.build/reference/be/common-definitions#common-attributes
    #
    # Copying file & directories is entirely IO-bound and there is no point doing this work
    # remotely.
    #
    # Also, remote-execution does not allow source directory inputs, see
    # https://github.com/bazelbuild/bazel/commit/c64421bc35214f0414e4f4226cc953e8c55fa0d2 So we must
    # not attempt to execute remotely in that case.
    #
    # There is also no point pulling the output file or directory from the remote cache since the
    # bytes to copy are already available locally. Conversely, no point in writing to the cache if
    # no one has any reason to check it for this action.
    #
    # Read and writing to disk cache is disabled as well primarily to reduce disk usage on the local
    # machine. A disk cache hit of a directory copy could be slghtly faster than a copy since the
    # disk cache stores the directory artifact as a single entry, but the slight performance bump
    # comes at the cost of heavy disk cache usage, which is an unmanaged directory that grow beyond
    # the bounds of the physical disk.
    "no-remote": "1",
    "no-cache": "1",
}

def copy_cmd(ctx, pairs):
    # Most Windows binaries built with MSVC use a certain argument quoting
    # scheme. Bazel uses that scheme too to quote arguments. However,
    # cmd.exe uses different semantics, so Bazel's quoting is wrong here.
    # To fix that we write the command to a .bat file so no command line
    # quoting or escaping is required.
    bat = ctx.actions.declare_file(ctx.label.name + "-cmd.bat")
    ctx.actions.write(
        output = bat,
        # Do not use lib/shell.bzl's shell.quote() method, because that uses
        # Bash quoting syntax, which is different from cmd.exe's syntax.
        content = "\n".join(["@copy /Y \"%s\" \"%s\" >NUL" % (
            src.path.replace("/", "\\"),
            dst.path.replace("/", "\\"),
        ) for (src,dst) in pairs]),
        is_executable = True,
    )
    srcs = [src for (src, _) in pairs]
    dsts = [dst for (_, dst) in pairs]
    ctx.actions.run(
        inputs = srcs,
        tools = [bat],
        outputs = dsts,
        executable = "cmd.exe",
        arguments = ["/C", bat.path.replace("/", "\\")],
        mnemonic = "CopyFile",
        progress_message = "Copying files",
        use_default_shell_env = True,
        execution_requirements = COPY_EXECUTION_REQUIREMENTS,
    )

def copy_bash(ctx, pairs):
    for (src, dst) in pairs:
        ctx.actions.run_shell(
            tools = [src],
            outputs = [dst],
            command = "cp -f \"$1\" \"$2\"",
            arguments = [src.path, dst.path],
            mnemonic = "CopyFile",
            progress_message = "Copying files",
            use_default_shell_env = True,
            execution_requirements = COPY_EXECUTION_REQUIREMENTS,
        )