package(default_visibility = ["//visibility:public"])

alias(
    name = "config",
    actual = select(
        {
            "//spool/platforms/STM32F401_MINI:platform": "//spool/platforms/STM32F401_MINI:config",
            "//spool/platforms/SKR_PRO_1_2:platform": "//spool/platforms/SKR_PRO_1_2:config",
            "//spool/platforms/SKR_MINI_E3_2_0:platform": "//spool/platforms/SKR_MINI_E3_2_0:config",
        },
        no_match_error = "No matching configuration found",
    ),
)

alias(
    name = "runtime",
    actual = select(
        {
            "//spool/platforms/STM32F401_MINI:platform": "//spool/platforms/STM32F401_MINI:runtime",
            "//spool/platforms/SKR_PRO_1_2:platform": "//spool/platforms/SKR_PRO_1_2:runtime",
            "//spool/platforms/SKR_MINI_E3_2_0:platform": "//spool/platforms/SKR_MINI_E3_2_0:runtime",

        },
        no_match_error = "No matching configuration found",
    )
)

alias(
    name = "linker.ld",
    actual = select(
        {
            "//spool/platforms/STM32F401_MINI:platform": "//spool/platforms/STM32F401_MINI:linker.ld",
            "//spool/platforms/SKR_PRO_1_2:platform": "//spool/platforms/SKR_PRO_1_2:linker.ld",
            "//spool/platforms/SKR_MINI_E3_2_0:platform": "//spool/platforms/SKR_MINI_E3_2_0:linker.ld",
        },
        no_match_error = "No matching configuration found",
    ),
)
