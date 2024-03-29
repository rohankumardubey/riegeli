// Copyright 2017 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef RIEGELI_VARINT_VARINT_READING_H_
#define RIEGELI_VARINT_VARINT_READING_H_

#include <stddef.h>
#include <stdint.h>

#include <type_traits>

#include "absl/base/optimization.h"
#include "absl/types/optional.h"
#include "riegeli/bytes/reader.h"
#include "riegeli/varint/varint_internal.h"

namespace riegeli {

// Unless stated otherwise, reading a varint tolerates representations
// which are not the shortest, but rejects representations longer than
// `kMaxLengthVarint{32,64}` bytes or with bits set outside the range of
// possible values.
//
// Warning: unless stated otherwise, reading a varint may read ahead more than
// eventually needed. If reading ahead only as much as needed is required, e.g.
// when communicating with another process, use `StreamingReadVarint{32,64}()`
// instead. It is slower.

// Reads a varint.
//
// Warning: the proto library writes values of type `int32` (not `sint32`) by
// casting them to `uint64`, not `uint32` (negative values take 10 bytes, not
// 5), hence they must be read with `ReadVarint64()`, not `ReadVarint32()`, if
// negative values are possible.
//
// Return values:
//  * `true`                          - success (`dest` is set)
//  * `false` (when `src.healthy()`)  - source ends too early
//                                      (`src` position is unchanged,
//                                      `dest` is undefined)
//  * `false` (when `!src.healthy()`) - failure
//                                      (`src` position is unchanged,
//                                      `dest` is undefined)
bool ReadVarint32(Reader& src, uint32_t& dest);
bool ReadVarint64(Reader& src, uint64_t& dest);

// Reads a varint.
//
// Accepts only the canonical representation, i.e. the shortest: rejecting a
// trailing zero byte, except for 0 itself.
//
// Return values:
//  * `true`                          - success (`dest` is set)
//  * `false` (when `src.healthy()`)  - source ends too early
//                                      (`src` position is unchanged,
//                                      `dest` is undefined)
//  * `false` (when `!src.healthy()`) - failure
//                                      (`src` position is unchanged,
//                                      `dest` is undefined)
bool ReadCanonicalVarint32(Reader& src, uint32_t& dest);
bool ReadCanonicalVarint64(Reader& src, uint64_t& dest);

// Reads a varint.
//
// Reads ahead only as much as needed, which can be required e.g. when
// communicating with another process. This is slower than
// `ReadVarint{32,64}()`.
//
// Return values:
//  * `true`                          - success (`dest` is set)
//  * `false` (when `src.healthy()`)  - source ends too early
//                                      (`src` position is unchanged,
//                                      `dest` is undefined)
//  * `false` (when `!src.healthy()`) - failure
//                                      (`src` position is unchanged,
//                                      `dest` is undefined)
bool StreamingReadVarint32(Reader& src, uint32_t& dest);
bool StreamingReadVarint64(Reader& src, uint64_t& dest);

// Reads a varint from an array.
//
// Return values:
//  * updated `src`   - success (`dest` is set)
//  * `absl::nullopt` - source ends (dest` is undefined)
absl::optional<const char*> ReadVarint32(const char* src, const char* limit,
                                         uint32_t& dest);
absl::optional<const char*> ReadVarint64(const char* src, const char* limit,
                                         uint64_t& dest);

// Copies a varint to an array.
//
// Writes up to `kMaxLengthVarint{32,64}` bytes to `dest[]`.
//
// Return values:
//  * varint length                           - success (`dest[]` is filled)
//  * `absl::nullopt` (when `src.healthy()`)  - source ends too early
//                                              (`src` position is unchanged,
//                                             `dest[]` is undefined)
//  * `absl::nullopt` (when `!src.healthy()`) - failure
//                                              (`src` position is unchanged,
//                                              `dest[]` is undefined)
absl::optional<size_t> CopyVarint32(Reader& src, char* dest);
absl::optional<size_t> CopyVarint64(Reader& src, char* dest);

// Copies a varint from an array to an array.
//
// Writes up to `kMaxLengthVarint{32,64}` bytes to `dest[]`.
//
// Return values:
//  * varint length   - success (`dest[]` is filled)
//  * `absl::nullopt` - source ends (`dest[]` is undefined)
absl::optional<size_t> CopyVarint32(const char* src, const char* limit,
                                    char* dest);
absl::optional<size_t> CopyVarint64(const char* src, const char* limit,
                                    char* dest);

// Implementation details follow.

inline bool ReadVarint32(Reader& src, uint32_t& dest) {
  if (ABSL_PREDICT_FALSE(src.available() < kMaxLengthVarint32)) {
    if (src.available() > 0 && static_cast<uint8_t>(src.limit()[-1]) < 0x80) {
      // The buffer contains a potential varint terminator. Avoid pulling the
      // maximum varint length which can be expensive.
    } else {
      src.Pull(kMaxLengthVarint32);
    }
  }
  const absl::optional<const char*> cursor =
      ReadVarint32(src.cursor(), src.limit(), dest);
  if (ABSL_PREDICT_FALSE(cursor == absl::nullopt)) return false;
  src.set_cursor(*cursor);
  return true;
}

inline bool ReadVarint64(Reader& src, uint64_t& dest) {
  if (ABSL_PREDICT_FALSE(src.available() < kMaxLengthVarint64)) {
    if (src.available() > 0 && static_cast<uint8_t>(src.limit()[-1]) < 0x80) {
      // The buffer contains a potential varint terminator. Avoid pulling the
      // maximum varint length which can be expensive.
    } else {
      src.Pull(kMaxLengthVarint64);
    }
  }
  const absl::optional<const char*> cursor =
      ReadVarint64(src.cursor(), src.limit(), dest);
  if (ABSL_PREDICT_FALSE(cursor == absl::nullopt)) return false;
  src.set_cursor(*cursor);
  return true;
}

inline bool ReadCanonicalVarint32(Reader& src, uint32_t& dest) {
  if (ABSL_PREDICT_FALSE(src.available() < kMaxLengthVarint32)) {
    if (src.available() > 0 && static_cast<uint8_t>(src.limit()[-1]) < 0x80) {
      // The buffer contains a potential varint terminator. Avoid pulling the
      // maximum varint length which can be expensive.
    } else {
      src.Pull(kMaxLengthVarint32);
    }
  }
  if (ABSL_PREDICT_FALSE(src.cursor() == src.limit())) return false;
  const uint8_t first_byte = static_cast<uint8_t>(*src.cursor());
  if (first_byte < 0x80) {
    // Any byte with the highest bit clear is accepted as the only byte,
    // including 0 itself.
    dest = first_byte;
    src.move_cursor(1);
    return true;
  }
  const absl::optional<const char*> cursor =
      ReadVarint32(src.cursor(), src.limit(), dest);
  if (ABSL_PREDICT_FALSE(cursor == absl::nullopt)) return false;
  if (ABSL_PREDICT_FALSE((*cursor)[-1] == 0)) return false;
  src.set_cursor(*cursor);
  return true;
}

inline bool ReadCanonicalVarint64(Reader& src, uint64_t& dest) {
  if (ABSL_PREDICT_FALSE(src.available() < kMaxLengthVarint64)) {
    if (src.available() > 0 && static_cast<uint8_t>(src.limit()[-1]) < 0x80) {
      // The buffer contains a potential varint terminator. Avoid pulling the
      // maximum varint length which can be expensive.
    } else {
      src.Pull(kMaxLengthVarint64);
    }
  }
  if (ABSL_PREDICT_FALSE(src.cursor() == src.limit())) return false;
  const uint8_t first_byte = static_cast<uint8_t>(*src.cursor());
  if (first_byte < 0x80) {
    // Any byte with the highest bit clear is accepted as the only byte,
    // including 0 itself.
    dest = first_byte;
    src.move_cursor(1);
    return true;
  }
  const absl::optional<const char*> cursor =
      ReadVarint64(src.cursor(), src.limit(), dest);
  if (ABSL_PREDICT_FALSE(cursor == absl::nullopt)) return false;
  if (ABSL_PREDICT_FALSE((*cursor)[-1] == 0)) return false;
  src.set_cursor(*cursor);
  return true;
}

namespace internal {

bool StreamingReadVarint32Slow(Reader& src, uint32_t& dest);
bool StreamingReadVarint64Slow(Reader& src, uint64_t& dest);

}  // namespace internal

inline bool StreamingReadVarint32(Reader& src, uint32_t& dest) {
  if (ABSL_PREDICT_FALSE(src.available() < kMaxLengthVarint32)) {
    if (ABSL_PREDICT_FALSE(!src.Pull(1, kMaxLengthVarint32))) {
      return false;
    }
    if (static_cast<uint8_t>(src.limit()[-1]) < 0x80) {
      // The buffer contains a potential varint terminator. Avoid pulling
      // repeatedly which can be expensive.
    } else if (ABSL_PREDICT_FALSE(src.available() < kMaxLengthVarint32)) {
      return internal::StreamingReadVarint32Slow(src, dest);
    }
  }
  const absl::optional<const char*> cursor =
      ReadVarint32(src.cursor(), src.limit(), dest);
  if (ABSL_PREDICT_FALSE(cursor == absl::nullopt)) return false;
  src.set_cursor(*cursor);
  return true;
}

inline bool StreamingReadVarint64(Reader& src, uint64_t& dest) {
  if (ABSL_PREDICT_FALSE(src.available() < kMaxLengthVarint64)) {
    if (ABSL_PREDICT_FALSE(!src.Pull(1, kMaxLengthVarint64))) {
      return false;
    }
    if (static_cast<uint8_t>(src.limit()[-1]) < 0x80) {
      // The buffer contains a potential varint terminator. Avoid pulling
      // repeatedly which can be expensive.
    } else if (ABSL_PREDICT_FALSE(src.available() < kMaxLengthVarint64)) {
      return internal::StreamingReadVarint64Slow(src, dest);
    }
  }
  const absl::optional<const char*> cursor =
      ReadVarint64(src.cursor(), src.limit(), dest);
  if (ABSL_PREDICT_FALSE(cursor == absl::nullopt)) return false;
  src.set_cursor(*cursor);
  return true;
}

namespace internal {

constexpr size_t kReadVarintSlowThreshold = 3 * 7;

absl::optional<const char*> ReadVarint32Slow(const char* src, const char* limit,
                                             uint32_t acc, uint32_t& dest);
absl::optional<const char*> ReadVarint64Slow(const char* src, const char* limit,
                                             uint64_t acc, uint64_t& dest);

}  // namespace internal

inline absl::optional<const char*> ReadVarint32(const char* src,
                                                const char* limit,
                                                uint32_t& dest) {
  if (ABSL_PREDICT_FALSE(src == limit)) return absl::nullopt;
  uint8_t byte = static_cast<uint8_t>(*src++);
  uint32_t acc{byte};
  size_t shift = 7;
  while (byte >= 0x80) {
    if (ABSL_PREDICT_FALSE(shift == internal::kReadVarintSlowThreshold)) {
      return internal::ReadVarint32Slow(src, limit, acc, dest);
    }
    if (ABSL_PREDICT_FALSE(src == limit)) return absl::nullopt;
    byte = static_cast<uint8_t>(*src++);
    acc += (uint32_t{byte} - 1) << shift;
    shift += 7;
  }
  dest = acc;
  return src;
}

inline absl::optional<const char*> ReadVarint64(const char* src,
                                                const char* limit,
                                                uint64_t& dest) {
  if (ABSL_PREDICT_FALSE(src == limit)) return absl::nullopt;
  uint8_t byte = static_cast<uint8_t>(*src++);
  uint64_t acc{byte};
  size_t shift = 7;
  while (byte >= 0x80) {
    if (ABSL_PREDICT_FALSE(shift == internal::kReadVarintSlowThreshold)) {
      return internal::ReadVarint64Slow(src, limit, acc, dest);
    }
    if (ABSL_PREDICT_FALSE(src == limit)) return absl::nullopt;
    byte = static_cast<uint8_t>(*src++);
    acc += (uint64_t{byte} - 1) << shift;
    shift += 7;
  }
  dest = acc;
  return src;
}

inline absl::optional<size_t> CopyVarint32(Reader& src, char* dest) {
  if (ABSL_PREDICT_FALSE(src.available() < kMaxLengthVarint32)) {
    if (src.available() > 0 && static_cast<uint8_t>(src.limit()[-1]) < 0x80) {
      // The buffer contains a potential varint terminator. Avoid pulling the
      // maximum varint length which can be expensive.
    } else {
      src.Pull(kMaxLengthVarint32);
    }
  }
  const absl::optional<size_t> length =
      CopyVarint32(src.cursor(), src.limit(), dest);
  if (ABSL_PREDICT_FALSE(length == absl::nullopt)) return absl::nullopt;
  src.move_cursor(*length);
  return *length;
}

inline absl::optional<size_t> CopyVarint64(Reader& src, char* dest) {
  if (ABSL_PREDICT_FALSE(src.available() < kMaxLengthVarint64)) {
    if (src.available() > 0 && static_cast<uint8_t>(src.limit()[-1]) < 0x80) {
      // The buffer contains a potential varint terminator. Avoid pulling the
      // maximum varint length which can be expensive.
    } else {
      src.Pull(kMaxLengthVarint64);
    }
  }
  const absl::optional<size_t> length =
      CopyVarint64(src.cursor(), src.limit(), dest);
  if (ABSL_PREDICT_FALSE(length == absl::nullopt)) return absl::nullopt;
  src.move_cursor(*length);
  return *length;
}

inline absl::optional<size_t> CopyVarint32(const char* src, const char* limit,
                                           char* dest) {
  if (ABSL_PREDICT_FALSE(src == limit)) return absl::nullopt;
  const char* start = src;
  uint8_t byte = static_cast<uint8_t>(*src++);
  *dest++ = static_cast<char>(byte);
  size_t remaining = kMaxLengthVarint32 - 1;
  while (byte >= 0x80) {
    if (ABSL_PREDICT_FALSE(src == limit)) return absl::nullopt;
    byte = static_cast<uint8_t>(*src++);
    *dest++ = static_cast<char>(byte);
    if (ABSL_PREDICT_FALSE(--remaining == 0)) {
      // Last possible byte.
      if (ABSL_PREDICT_FALSE(
              byte >= uint8_t{1} << (32 - (kMaxLengthVarint32 - 1) * 7))) {
        // The representation is longer than `kMaxLengthVarint32`
        // or the represented value does not fit in `uint32_t`.
        return absl::nullopt;
      }
      break;
    }
  }
  return PtrDistance(start, src);
}

inline absl::optional<size_t> CopyVarint64(const char* src, const char* limit,
                                           char* dest) {
  if (ABSL_PREDICT_FALSE(src == limit)) return absl::nullopt;
  const char* start = src;
  uint8_t byte = static_cast<uint8_t>(*src++);
  *dest++ = static_cast<char>(byte);
  size_t remaining = kMaxLengthVarint64 - 1;
  while (byte >= 0x80) {
    if (ABSL_PREDICT_FALSE(src == limit)) return absl::nullopt;
    byte = static_cast<uint8_t>(*src++);
    *dest++ = static_cast<char>(byte);
    if (ABSL_PREDICT_FALSE(--remaining == 0)) {
      // Last possible byte.
      if (ABSL_PREDICT_FALSE(
              byte >= uint8_t{1} << (64 - (kMaxLengthVarint64 - 1) * 7))) {
        // The representation is longer than `kMaxLengthVarint64`
        // or the represented value does not fit in `uint64_t`.
        return absl::nullopt;
      }
      break;
    }
  }
  return PtrDistance(start, src);
}

}  // namespace riegeli

#endif  // RIEGELI_VARINT_VARINT_READING_H_
