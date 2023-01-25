cc_library(
    name="lib",
    hdrs=glob(["src/*.h",]),
    srcs=glob(["src/*.c",]),
    strip_include_prefix="src",
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