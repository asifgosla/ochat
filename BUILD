cc_binary(
    name = "ochat",
    srcs = [
        "app_config.h",
        "ochat.h",
        "ochat.cpp",
    ],
 #   copts = [""],
    deps = [
        "@boost.json//:boost.json",
        "@boost.asio//:boost.asio",
    ],
)

cc_library(
    name = "ochat_lib",
    srcs = [
        "ochat.cpp",
    ],
    hdrs = ["app_config.h", "ochat.h"],
    defines = ["LIBRARY_BUILD"],
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