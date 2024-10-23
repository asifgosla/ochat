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
    #copts = ["-fno-inline"],
    #copts = ["-fweak","-g","-O0"],
    defines = [],
    linkstatic = True,  # Forces static linking
    deps = [
        "@boost.json//:boost.json",
        "@boost.asio//:boost.asio", 
    ],
)
cc_test(
    name = "ochat_test",
    srcs = [
        "test/ochat_test.cpp",
        "test/ochat_socket_test.cpp",
        "test/mock_asio.h",
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