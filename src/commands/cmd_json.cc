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
#include "scan_base.h"
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
    redis_json.Get(key, json_paths, output);

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
    std::string_view set_flag;
    // TODO(mwish): These are all not support yet.
    while (parser.Good()) {
      if (parser.EatEqICaseFlag("NX", set_flag)) {
        return Status::RedisParseErr;
      } else if (parser.EatEqICaseFlag("XX", set_flag)) {
        return Status::RedisParseErr;
      } else if (parser.EatEqICaseFlag("FORMAT", set_flag)) {
        return Status::RedisParseErr;
      } else {
        return parser.InvalidSyntax();
      }
    }

    return Status::OK();
  }

  Status Execute(Server *svr, Connection *conn, std::string *output) override {}

 private:
};

///
/// JSON.DEL <key> [path]
///
class CommandJsonDel final : public CommandJsonBase {
  Status Execute(Server *svr, Connection *conn, std::string *output) override {
    // NYI
    __builtin_unreachable();
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