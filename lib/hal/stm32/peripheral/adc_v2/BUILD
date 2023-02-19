cc_library(
    name="lib",
    srcs=glob(["*.c"]) + ["adc_private.h"],
    hdrs=["adc.h"],
    include_prefix="hal/stm32",
    alwayslink=True,
    deps=[
        "//lib/common:lib",
        "//lib/freertos:freertos",
        "//lib/hal/common:lib",
        "//lib/hal/cortexm:lib",
        "//lib/hardware_manual:manual",
    ],
    visibility=["//lib/hal:__subpackages__"],
)
