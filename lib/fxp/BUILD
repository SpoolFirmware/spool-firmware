cc_library(
    name="lib",
    hdrs=glob(["src/*.h",]),
    srcs=[
      "src/fix16.c",
    ],
    strip_include_prefix="src",
    visibility = ["//visibility:public"],
)

cc_test(
  name = "simple_test",
  size = "small",
  srcs = ["test/simple.c"],
  deps = [
    ":lib", 
    "//lib/munit:lib"
  ],
)

test_suite(
    name = "tests",
    tags = [],
)
