package(default_visibility = ["//visibility:public"])

licenses(["notice"])

cc_library(
    name = "zstd_writer",
    srcs = ["zstd_writer.cc"],
    hdrs = ["zstd_writer.h"],
    deps = [
        ":zstd_dictionary",
        "//riegeli/base",
        "//riegeli/base:recycling_pool",
        "//riegeli/base:status",
        "//riegeli/bytes:buffered_writer",
        "//riegeli/bytes:writer",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:optional",
        "@net_zstd//:zstdlib",
    ],
)

cc_library(
    name = "zstd_reader",
    srcs = ["zstd_reader.cc"],
    hdrs = ["zstd_reader.h"],
    deps = [
        ":zstd_dictionary",
        "//riegeli/base",
        "//riegeli/base:recycling_pool",
        "//riegeli/base:status",
        "//riegeli/bytes:buffered_reader",
        "//riegeli/bytes:reader",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:optional",
        "@net_zstd//:zstdlib",
    ],
)

cc_library(
    name = "zstd_dictionary",
    srcs = ["zstd_dictionary.cc"],
    hdrs = ["zstd_dictionary.h"],
    visibility = ["//visibility:private"],
    deps = [
        "//riegeli/base",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/synchronization",
        "@net_zstd//:zstdlib",
    ],
)
