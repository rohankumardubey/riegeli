// Copyright 2020 Google LLC
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

#include "riegeli/csv/csv_reader.h"

#include <stddef.h>

#include <array>
#include <string>
#include <vector>

#include "absl/base/optimization.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/types/optional.h"
#include "riegeli/base/base.h"
#include "riegeli/bytes/reader.h"

namespace riegeli {

void CsvReaderBase::Initialize(Reader* src, Options&& options) {
  RIEGELI_ASSERT(src != nullptr)
      << "Failed precondition of CsvReader: null Reader pointer";
  RIEGELI_ASSERT(options.escape() != options.field_separator())
      << "Escape character conflicts with field separator";
  if (ABSL_PREDICT_FALSE(!src->healthy())) {
    Fail(*src);
    return;
  }

  char_classes_['\n'] = CharClass::kLf;
  char_classes_['\r'] = CharClass::kCr;
  char_classes_[static_cast<unsigned char>(options.field_separator())] =
      CharClass::kFieldSeparator;
  char_classes_['"'] = CharClass::kQuote;
  if (options.escape() != absl::nullopt) {
    char_classes_[static_cast<unsigned char>(*options.escape())] =
        CharClass::kEscape;
  }
  max_num_fields_ = UnsignedMin(options.max_num_fields(),
                                std::vector<std::string>().max_size());
  max_field_length_ =
      UnsignedMin(options.max_field_length(), std::string().max_size());
}

bool CsvReaderBase::MaxFieldLengthExceeded() {
  return Fail(absl::ResourceExhaustedError(
      absl::StrCat("Maximum field length exceeded: ", max_field_length_)));
}

inline bool CsvReaderBase::ReadQuoted(Reader& src, std::string& field) {
  if (ABSL_PREDICT_FALSE(!field.empty())) {
    return Fail(absl::DataLossError("Unquoted data before opening quote"));
  }

  // Data from `src.cursor()` to where `ptr` stops will be appended to `field`.
  const char* ptr = src.cursor();
  for (;;) {
    if (ABSL_PREDICT_FALSE(ptr == src.limit())) {
      if (ABSL_PREDICT_FALSE(src.available() >
                             max_field_length_ - field.size())) {
        return MaxFieldLengthExceeded();
      }
      field.append(src.cursor(), src.available());
      src.move_cursor(src.available());
      if (ABSL_PREDICT_FALSE(!src.Pull())) {
        if (ABSL_PREDICT_FALSE(!src.healthy())) return Fail(src);
        return Fail(absl::DataLossError("Missing closing quote"));
      }
      ptr = src.cursor();
    }
    const CharClass char_class =
        char_classes_[static_cast<unsigned char>(*ptr++)];
    if (ABSL_PREDICT_TRUE(char_class == CharClass::kOther) ||
        char_class == CharClass::kLf || char_class == CharClass::kCr ||
        char_class == CharClass::kFieldSeparator) {
      continue;
    }
    const size_t length = PtrDistance(src.cursor(), ptr - 1);
    if (ABSL_PREDICT_FALSE(length > max_field_length_ - field.size())) {
      return MaxFieldLengthExceeded();
    }
    field.append(src.cursor(), length);
    src.set_cursor(ptr);
    switch (char_class) {
      case CharClass::kOther:
      case CharClass::kLf:
      case CharClass::kCr:
      case CharClass::kFieldSeparator:
        RIEGELI_ASSERT_UNREACHABLE() << "Handled before switch";
      case CharClass::kQuote:
        if (ABSL_PREDICT_FALSE(!src.Pull())) {
          if (ABSL_PREDICT_FALSE(!src.healthy())) return Fail(src);
          return true;
        }
        if (*src.cursor() == '"') {
          // Quote written twice.
          ptr = src.cursor() + 1;
          continue;
        }
        return true;
      case CharClass::kEscape:
        if (ABSL_PREDICT_FALSE(!src.Pull())) {
          if (ABSL_PREDICT_FALSE(!src.healthy())) return Fail(src);
          return Fail(absl::DataLossError("Missing character after escape"));
        }
        ptr = src.cursor() + 1;
        continue;
    }
    RIEGELI_ASSERT_UNREACHABLE()
        << "Unknown character class: " << static_cast<int>(char_class);
  }
}

inline bool CsvReaderBase::ReadFields(Reader& src,
                                      std::vector<std::string>& fields,
                                      size_t& index) {
  RIEGELI_ASSERT_EQ(index, 0)
      << "Failed precondition of CsvReaderBase::ReadFields(): "
         "initial index must be 0";
  if (ABSL_PREDICT_FALSE(!src.Pull())) {
    // End of file at the beginning of a record.
    if (ABSL_PREDICT_FALSE(!src.healthy())) return Fail(src);
    return false;
  }

next_field:
  if (ABSL_PREDICT_FALSE(index == max_num_fields_)) {
    return Fail(absl::ResourceExhaustedError(
        absl::StrCat("Maximum number of fields exceeded: ", max_num_fields_)));
  }
  if (fields.size() == index) {
    fields.emplace_back();
  } else {
    fields[index].clear();
  }
  std::string& field = fields[index];

  // Data from `src.cursor()` to where `ptr` stops will be appended to `field`.
  const char* ptr = src.cursor();
  for (;;) {
    if (ABSL_PREDICT_FALSE(ptr == src.limit())) {
      if (ABSL_PREDICT_FALSE(src.available() >
                             max_field_length_ - field.size())) {
        return MaxFieldLengthExceeded();
      }
      field.append(src.cursor(), src.available());
      src.move_cursor(src.available());
      if (ABSL_PREDICT_FALSE(!src.Pull())) {
        if (ABSL_PREDICT_FALSE(!src.healthy())) return Fail(src);
        return true;
      }
      ptr = src.cursor();
    }
    const CharClass char_class =
        char_classes_[static_cast<unsigned char>(*ptr++)];
    if (ABSL_PREDICT_TRUE(char_class == CharClass::kOther) ||
        char_class == CharClass::kEscape) {
      continue;
    }
    const size_t length = PtrDistance(src.cursor(), ptr - 1);
    if (ABSL_PREDICT_FALSE(length > max_field_length_ - field.size())) {
      return MaxFieldLengthExceeded();
    }
    field.append(src.cursor(), length);
    src.set_cursor(ptr);
    switch (char_class) {
      case CharClass::kOther:
      case CharClass::kEscape:
        RIEGELI_ASSERT_UNREACHABLE() << "Handled before switch";
      case CharClass::kLf:
        return true;
      case CharClass::kCr:
        if (ABSL_PREDICT_FALSE(!src.Pull())) {
          if (ABSL_PREDICT_FALSE(!src.healthy())) return Fail(src);
          return true;
        }
        if (*src.cursor() == '\n') src.move_cursor(1);
        return true;
      case CharClass::kFieldSeparator:
        ++index;
        goto next_field;
      case CharClass::kQuote:
        if (ABSL_PREDICT_FALSE(!ReadQuoted(src, field))) return false;
        if (ABSL_PREDICT_FALSE(!src.Pull())) {
          if (ABSL_PREDICT_FALSE(!src.healthy())) return Fail(src);
          return true;
        }
        const CharClass char_class_after_quoted =
            char_classes_[static_cast<unsigned char>(*src.cursor())];
        src.move_cursor(1);
        switch (char_class_after_quoted) {
          case CharClass::kOther:
          case CharClass::kEscape:
            return Fail(
                absl::DataLossError("Unquoted data after closing quote"));
          case CharClass::kFieldSeparator:
            ++index;
            goto next_field;
          case CharClass::kLf:
            return true;
          case CharClass::kCr:
            if (ABSL_PREDICT_FALSE(!src.Pull())) {
              if (ABSL_PREDICT_FALSE(!src.healthy())) return Fail(src);
              return true;
            }
            if (*src.cursor() == '\n') src.move_cursor(1);
            return true;
          case CharClass::kQuote:
            RIEGELI_ASSERT_UNREACHABLE() << "Handled by ReadQuoted()";
        }
        RIEGELI_ASSERT_UNREACHABLE()
            << "Unknown character class: "
            << static_cast<int>(char_class_after_quoted);
    }
    RIEGELI_ASSERT_UNREACHABLE()
        << "Unknown character class: " << static_cast<int>(char_class);
  }
}

bool CsvReaderBase::ReadRecord(std::vector<std::string>& fields) {
  if (ABSL_PREDICT_FALSE(!healthy())) {
    fields.clear();
    return false;
  }
  Reader& src = *src_reader();
  size_t index = 0;
  // Assign to existing elements of `fields` when possible and then `erase()`
  // excess elements, instead of calling `fields.clear()` upfront, to avoid
  // losing existing `std::string` allocations.
  if (ABSL_PREDICT_FALSE(!ReadFields(src, fields, index))) {
    fields.clear();
    return false;
  }
  fields.erase(fields.begin() + index + 1, fields.end());
  return true;
}

}  // namespace riegeli
