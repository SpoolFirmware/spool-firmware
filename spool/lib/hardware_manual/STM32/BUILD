load("//spool/lib/hardware_manual/STM32:header.bzl", "hardware_manual_library")

filegroup(
    name="peripheral_patches",
    srcs=glob(["patches/devices/**/*.yaml","patches/peripherals/**/*.yaml"])
)

hardware_manual_library(name = "stm32f407")
hardware_manual_library(name = "stm32f401")
