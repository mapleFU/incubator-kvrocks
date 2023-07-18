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

namespace redis {

rocksdb::Status RedisJson::JsonDel(const Slice &user_key, const JsonPath &path) { return rocksdb::Status::OK(); }
rocksdb::Status RedisJson::JsonGet(const Slice &user_key, const std::vector<JsonPath> &path, std::string *values) {
  return rocksdb::Status::OK();
}
rocksdb::Status RedisJson::JsonSet(const Slice &user_key, const JsonPath &path, const Slice &set_value,
                                   JsonSetFlags set_flags) {
  jsoncons::json_parser parser;
  parser.update(set_value.ToStringView());
  std::error_code ec;
  jsoncons::json_decoder<JsonType> json_decoder;
  parser.finish_parse(json_decoder, ec);
  if (!json_decoder.is_valid()) {
    return rocksdb::Status::IOError(ec.message());
  }
  auto json = json_decoder.get_result();
  rocksdb::ReadOptions read_options;
  std::string exist_key;
  auto s = storage_->Get(read_options, user_key, &exist_key);
  if (!s.ok() && !s.IsNotFound()) {
    return s;
  }
  if (s.IsNotFound()) {
    if (set_flags == JsonSetFlags::kJsonSetXX) {
      // set a normalized json string
      // NYI
      __builtin_unreachable();
    } else {
      // NYI
      __builtin_unreachable();
    }
  }
  return {};
}

}  // namespace redis