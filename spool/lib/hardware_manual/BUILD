alias(
    name="mcu",
    actual=select({
        "//spool/platforms/mcu:stm32f401": "//spool/lib/hardware_manual/STM32:stm32f401",
        # "//spool/platforms/mcu:stm32f407": ":stm32f407_header",
    },
    no_match_error="No matching MCU configuration found"),
    visibility = ["//visibility:public"]
)

