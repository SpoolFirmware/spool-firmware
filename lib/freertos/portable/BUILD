package(default_visibility=["//visibility:public"])

alias(
    name = "lib",
    actual = select(
        {
            "@bazel_embedded//constraints/cpu_variant:cortex_m4f": "//lib/freertos/portable/GCC/ARM_CM4F:port",            
            "@bazel_embedded//constraints/cpu_variant:cortex_m3": "//lib/freertos/portable/GCC/ARM_CM3:port",
        },
        no_match_error = "No matching FreeRTOS Port found",
    )
)