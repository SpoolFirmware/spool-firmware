package(default_visibility=["//visibility:public"])

constraint_setting(
    name = "port"
)

constraint_value(
    name = "arm_cm4f",
    constraint_setting = ":port",
)

constraint_value(
    name = "arm_cm3",
    constraint_setting = ":port",
)

alias(
    name = "lib",
    actual = select(
        {
            ":arm_cm4f": "//lib/freertos/portable/GCC/ARM_CM4F:port",            
            ":arm_cm3": "//lib/freertos/portable/GCC/ARM_CM3:port",
        },
        no_match_error = "No matching FreeRTOS Port found",
    )
)