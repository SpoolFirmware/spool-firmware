cc_library(
    name="base",
    srcs=glob([
        "src/**/*.c",
        "src/**/*.h",
        "fonts/*.c",
    ]) + [
        "lvgl.h",
    ],
    deps=[
        "//lib/lvgl/config:lib",
    ],
    visibility=["//lib/lvgl:__pkg__"],
)
