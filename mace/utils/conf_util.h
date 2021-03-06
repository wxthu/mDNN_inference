// Copyright 2018 The MACE Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef MACE_UTILS_CONF_UTIL_H_
#define MACE_UTILS_CONF_UTIL_H_

#include <algorithm>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

namespace mace {

inline bool EnvConfEnabled(std::string env_name) {
  char *env = getenv(env_name.c_str());
  return !(!env || env[0] == 0 || env[0] == '0');
}

inline int GetIntEnv(const std::string &name, int default_value) {
  char *env = getenv(name.c_str());
  return env != nullptr ? std::atoi(env) : default_value;
}

}  // namespace mace

#endif  // MACE_UTILS_CONF_UTIL_H_
