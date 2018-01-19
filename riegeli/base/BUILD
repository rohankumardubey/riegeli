package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "base",
    srcs = [
        "assert.cc",
        "object.cc",
        "port.h",
        "string_view.cc",
    ],
    hdrs = [
        "assert.h",
        "base.h",
        "memory.h",
        "object.h",
        "string_view.h",
    ],
)

cc_library(
    name = "memory_estimator",
    srcs = ["memory_estimator.cc"],
    hdrs = ["memory_estimator.h"],
    deps = [":base"],
)

cc_library(
    name = "chain",
    srcs = ["chain.cc"],
    hdrs = ["chain.h"],
    deps = [
        ":base",
        ":memory_estimator",
    ],
)

cc_library(
    name = "endian",
    hdrs = ["endian.h"],
)

cc_library(
    name = "parallelism",
    srcs = ["parallelism.cc"],
    hdrs = ["parallelism.h"],
    visibility = ["//riegeli:__subpackages__"],
    deps = [":base"],
)