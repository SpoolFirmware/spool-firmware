cc_library(
    name="cortex_m4f",
    srcs=["crt0_cm4f.s", "crt1.c"],
)

cc_library(
    name="cortex_m3",
    srcs=["crt0_cm3.s", "crt1.c"],
)

alias(
    name = "lib",
    actual = select(
        {
            "@bazel_embedded//constraints/cpu_variant:cortex_m4f": ":cortex_m4f",            
            "@bazel_embedded//constraints/cpu_variant:cortex_m3": ":cortex_m3",
        },
        no_match_error = "No matching FreeRTOS Port found",
    ),
    visibility = ["//visibility:public"]
)