# Building
```
bazel build //:spool
```
builds all the targets

```
bazel build //spool:stm32f401_mini
```
builds the bin and elf files for target stm32f401_mini

# Add boards
Modify `//spool/BUILD`, `//spool/platforms/BUILD` to add the new targets.

Then make the new directory `//spool/platforms/{board}`.

# Set up clangd
```
bazel run //:refresh_compile_commands
```

Follow the setup guide here:

https://github.com/SpoolFirmware/bazel-compile-commands-extractor#editor-setup--for-autocomplete-based-on-compile_commandsjson

### bad docs somewhere is better than good docs nowhere