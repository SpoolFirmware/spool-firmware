constraint_setting(
    name = "lvgl_config",
    visibility = ["//visibility:public"],
)

constraint_value(
    name = "12864",
    constraint_setting = ":lvgl_config",
    visibility = ["//visibility:public"],
)

alias(
    name = "lib",
    actual = select(
        {
            ":12864": "//lib/lvgl/config/12864:config",
        },
        no_match_error = "No matching lv_conf.h constaint found",
    ),
    visibility = ["//lib/lvgl/lib:__pkg__"],
)
