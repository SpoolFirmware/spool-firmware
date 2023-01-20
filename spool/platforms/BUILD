package(default_visibility = ["//visibility:public"])

cc_library(
    name = "lib",
    hdrs = ["platform.h"],
    include_prefix = "platform",
    visibility = ["//spool/platforms:__subpackages__"],
)

alias(
    name = "config",
    actual = select(
        {
            "//spool/platforms/STM32F401_MINI:platform": "//spool/platforms/STM32F401_MINI:config",
            "//spool/platforms/SKR_PRO_1_2:platform": "//spool/platforms/SKR_PRO_1_2:config",
        },
        no_match_error = "No matching configuration found",
    ),
)

alias(
    name = "runtime",
    actual = select(
        {
            "//spool/platforms/STM32F401_MINI:platform": "//spool/platforms/STM32F401_MINI:runtime",
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
        },
        no_match_error = "No matching configuration found",
    ),
)
