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

#include "commander.h"
#include "commands/command_parser.h"
#include "error_constants.h"
#include "server/server.h"
#include "types/json.h"
#include "types/redis_json.h"

namespace redis {

class CommandJsonBase : public Commander {
 public:
  inline static constexpr std::string_view CMD_ARG_NOESCAPE = "noescape";
  inline static constexpr std::string_view CMD_ARG_INDENT = "indent";
  inline static constexpr std::string_view CMD_ARG_NEWLINE = "newline";
  inline static constexpr std::string_view CMD_ARG_SPACE = "space";
  inline static constexpr std::string_view CMD_ARG_FORMAT = "format";

  inline static constexpr size_t JSONGET_SUBCOMMANDS_MAXSTRLEN = 8;

  inline static const std::unordered_set<std::string_view> INTERNAL_COMMANDS = {
      CMD_ARG_NOESCAPE, CMD_ARG_INDENT, CMD_ARG_NEWLINE, CMD_ARG_SPACE, CMD_ARG_FORMAT};
};

///
/// JSON.GET <key>
///         [path ...]
///
/// FORMAT, INDENT, NEWLINE, SPACE is not supported.
class CommandJsonGet final : public CommandJsonBase {
 public:
  Status Execute(Server *svr, Connection *conn, std::string *output) override {
    auto key = args_[0];
    std::vector<JsonPath> json_paths;
    for (size_t i = 1; i < args_.size(); ++i) {
      if (args_[i].size() < CommandJsonBase::JSONGET_SUBCOMMANDS_MAXSTRLEN) {
        auto lower = util::ToLower(args_[i]);
        if (INTERNAL_COMMANDS.find(lower) != INTERNAL_COMMANDS.end()) {
          if (lower == CMD_ARG_NOESCAPE) {
            continue;
          }
          // Not support
          return Status::FromErrno("NYI");
        }
      }
      auto json_path = JsonPath::BuildJsonPath(args_[i]);
      if (!json_path.IsOK()) {
        return json_path.ToStatus();
      }
      json_paths.push_back(std::move(json_path.GetValue()));
    }

    if (json_paths.empty()) {
      json_paths.push_back(JsonPath::BuildJsonPath("$").GetValue());
    }

    redis::RedisJson redis_json(svr->storage, conn->GetNamespace());
    rocksdb::Status s = redis_json.JsonGet(key, json_paths, output);
    if (!s.ok()) {
      return Status::FromErrno(s.ToString());
    }
    return Status::OK();
  }
};

///
/// JSON.SET <key> <path> <json> [NX | XX | FORMAT <format>]
///
class CommandJsonSet final : public CommandJsonBase {
 public:
  Status Parse(const std::vector<std::string> &args) override {
    CommandParser parser(args, 3);
    while (parser.Good()) {
      // TODO(mwish): make clear what's EatEqICase and EatEqICaseFlag.
      // TODO(mwish): verify multiple NX and XX used.
      if (parser.EatEqICase("NX")) {
        if (set_flags_ != JsonSetFlags::kNone) {
          return Status::RedisParseErr;
        }
        set_flags_ = JsonSetFlags::kJsonSetNX;
      } else if (parser.EatEqICase("XX")) {
        if (set_flags_ != JsonSetFlags::kNone) {
          return Status::RedisParseErr;
        }
        set_flags_ = JsonSetFlags::kJsonSetXX;
      } else if (parser.EatEqICase("FORMAT")) {
        // not support.
        // TODO(mwish): make clear what's the better string here.
        return Status::RedisParseErr;
      } else {
        // "ERR syntax error"
        return parser.InvalidSyntax();
      }
    }

    return Status::OK();
  }

  Status Execute(Server *svr, Connection *conn, std::string *output) override {
    auto json_path = JsonPath::BuildJsonPath(args_[2]);
    if (!json_path.IsOK()) {
      return json_path.ToStatus();
    }
    redis::RedisJson redis_json(svr->storage, conn->GetNamespace());
    rocksdb::Status s = redis_json.JsonSet(args_[1], json_path.GetValue(), args_[3], set_flags_);
    if (!s.ok()) {
      return Status::FromErrno(s.ToString());
    }
    return Status::OK();
  }

 private:
  JsonSetFlags set_flags_ = JsonSetFlags::kNone;
};

///
/// JSON.DEL <key> [path]
///
class CommandJsonDel final : public CommandJsonBase {
  Status Execute(Server *svr, Connection *conn, std::string *output) override {
    std::string_view json_path_string;
    if (args_.size() < 2) {
      // TODO(mwish): Use macro or other.
      json_path_string = "$";
    } else {
      json_path_string = args_[1];
    }

    auto json_path = JsonPath::BuildJsonPath(std::string(json_path_string));
    if (!json_path.IsOK()) {
      return json_path.ToStatus();
    }

    redis::RedisJson redis_json(svr->storage, conn->GetNamespace());
    rocksdb::Status s = redis_json.JsonDel(args_[1], json_path.GetValue());
    if (!s.ok()) {
      return Status::FromErrno(s.ToString());
    }
    return Status::OK();
  }
};

class CommandJsonType final : public CommandJsonBase {
  Status Execute(Server *svr, Connection *conn, std::string *output) override {
    // NYI
    __builtin_unreachable();
  }
};

REDIS_REGISTER_COMMANDS(MakeCmdAttr<CommandJsonDel>("json.del", -2, "write", 1, 1, 1),
                        MakeCmdAttr<CommandJsonGet>("json.get", -2, "read-only", 1, 1, 1),
                        MakeCmdAttr<CommandJsonSet>("json.set", -4, "write", 1, -2, 1),
                        MakeCmdAttr<CommandJsonType>("json.type", 3, "read-only", 1, 1, 1), )

}  // namespace redis