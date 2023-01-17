genrule(
    name="drf_stm32f401",
    srcs=["stm32f401.svd"],
    outs=["stm32f401.h"],
    tools=["//tools/svd2drf:svd2drf"],
    cmd="$(location //tools/svd2drf:svd2drf) -o $@ $<"
)