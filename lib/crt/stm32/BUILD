package(default_visibility=["//visibility:public"])

cc_library(
    name="stm32f4xx",
    srcs=["vectors_240.s"],
    deps=["//lib/crt/cortex_m:cortex_m4f"],
)

cc_library(
    name="stm32f1xx",
    srcs=["vectors_68.s"],
    deps=["//lib/crt/cortex_m:cortex_m3"],
)
