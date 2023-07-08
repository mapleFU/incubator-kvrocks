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
  return rocksdb::Status::OK();
}

}  // namespace redis