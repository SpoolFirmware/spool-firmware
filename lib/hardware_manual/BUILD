package(default_visibility = ["//visibility:public"])

constraint_setting(
    name = "mcu",
    visibility = ["//visibility:public"],
)

constraint_value(
    name = "stm32f103",
    constraint_setting = ":mcu",
)

constraint_value(
    name = "stm32f401",
    constraint_setting = ":mcu",
)

constraint_value(
    name = "stm32f407",
    constraint_setting = ":mcu",
)

alias(
    name="manual",
    actual=select({
        ":stm32f103": "//lib/hardware_manual/STM32:stm32f103",
        ":stm32f401": "//lib/hardware_manual/STM32:stm32f401",
        ":stm32f407": "//lib/hardware_manual/STM32:stm32f407",
    },
    no_match_error="No matching MCU configuration found"),
    visibility = ["//visibility:public"]
)

