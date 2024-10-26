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
    name = "ochat_format_test",
    srcs = [
        "test/ochat_format_test.cpp",
        "test/ochat_test_f.h"
    ],
    defines = ["UNIT_TESTING"],
    deps = [
        ":ochat_lib",
        "@googletest//:gtest", 
        "@googletest//:gtest_main",
    ],
    size = "small",
)
cc_test(
    name = "ochat_asio_test",
    srcs = [
        "test/ochat_asio_test.cpp",
        "test/mock_asio.h",
        "test/ochat_test_f.h"
    ],
    defines = ["UNIT_TESTING"],
    deps = [
        ":ochat_lib",
        "@googletest//:gtest", 
        "@googletest//:gtest_main",
    ],
    size = "small",
)