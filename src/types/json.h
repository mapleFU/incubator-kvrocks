#pragma once

#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonpath/jsonpath.hpp>
#include <jsoncons_ext/jsonpath/jsonpath_error.hpp>
#include <string>

#include "encoding.h"
#include "storage/redis_db.h"
#include "storage/redis_metadata.h"

namespace redis {

using JsonType = jsoncons::basic_json<char, jsoncons::sorted_policy>;
using JsonPathExpression = jsoncons::jsonpath::jsonpath_expression<JsonType>;

class JsonPath {
 public:
  static constexpr std::string_view ROOT_PATH = "$";

  static StatusOr<JsonPath> BuildJsonPath(std::string path);
  static JsonPath BuildJsonFullPath() { return BuildJsonPath(std::string(ROOT_PATH)).GetValue(); }

  bool IsLegacy() const noexcept { return !fixed_path_.empty(); }

  std::string_view Path() {
    if (IsLegacy()) {
      return fixed_path_;
    }
    return origin_;
  }

 private:
  static std::optional<std::string> tryConvertLegacyToJsonPath(Slice path);

  JsonPath(std::string path, std::string fixed_path, JsonPathExpression path_expression)
      : origin_(std::move(path)), fixed_path_(std::move(fixed_path)), expression_(std::move(path_expression)) {}

  std::string origin_;
  std::string fixed_path_;
  JsonPathExpression expression_;
};

enum class JsonSetFlags { kNone, kJsonSetNX, kJsonSetXX };

}  // namespace redis