/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 */

#include "redis_json.h"

#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonpath/jsonpath.hpp>
#include <jsoncons_ext/jsonpath/jsonpath_error.hpp>
#include <string>

namespace redis {

// TODO(mwish): Should I move it to separate place like `jsonpath.h` and `jsonpath.cc`?
using JsonType = jsoncons::basic_json<char, jsoncons::sorted_policy>;
using JsonPathExpression = jsoncons::jsonpath::jsonpath_expression<JsonType>;

class JsonPath {
 public:
  static StatusOr<JsonPath> BuildJsonPath(std::string path) {
    std::string fixed_path;
    std::string_view json_string;
    auto converted = tryConvertLegacyToJsonPath(path);
    if (converted.has_value()) {
      fixed_path = std::move(converted.value());
    }
    if (fixed_path.empty()) {
      json_string = path;
    } else {
      json_string = fixed_path;
    }

    std::error_code json_parse_error;
    auto path_expression = jsoncons::jsonpath::make_expression<JsonType>(json_string, json_parse_error);
    if (json_parse_error) {
      return Status::FromErrno(json_parse_error.message());
    }
    return JsonPath(std::move(path), std::move(fixed_path), std::move(path_expression));
  }

  bool IsLegacy() const noexcept { return !fixed_path_.empty(); }

  Slice Path() {
    if (IsLegacy()) {
      return fixed_path_;
    }
    return origin_;
  }

 private:
  // https://redis.io/docs/data-types/json/path/#legacy-path-syntax
  static std::optional<std::string> tryConvertLegacyToJsonPath(Slice path) {
    // NOT implemented yet
    __builtin_unreachable();
  }

  JsonPath(std::string path, std::string fixed_path, JsonPathExpression path_expression)
      : origin_(std::move(path)), fixed_path_(std::move(fixed_path)), expression_(std::move(path_expression)) {}

  std::string origin_;
  std::string fixed_path_;
  JsonPathExpression expression_;
};

}  // namespace redis