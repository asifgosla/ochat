cc_binary(
    name = "ochat",
    srcs = [
        "main.cpp",
    ],
 #   copts = [""],
    deps = [
        ":ochat_lib",
        "@boost.json//:boost.json",
        "@boost.asio//:boost.asio",
    ],
)

cc_library(
    name = "ochat_lib",
    srcs = [
        "ochat.cpp",
        "app_config.h",
    ],
    hdrs = ["app_config.h", "ochat.h"],
    defines = [],
    deps = [
        "@boost.json//:boost.json",
        "@boost.asio//:boost.asio", 
    ],
)
cc_test(
    name = "ochat_test",
    srcs = [
        "ochat_test.cpp",
    ],
    deps = [
        ":ochat_lib",
        "@boost.json//:boost.json",
        "@boost.asio//:boost.asio",
        "@googletest//:gtest", 
        "@googletest//:gtest_main",
    ],
    size = "small",
)