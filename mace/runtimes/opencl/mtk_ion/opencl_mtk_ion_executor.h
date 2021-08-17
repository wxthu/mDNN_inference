// Copyright 2021 The MACE Authors. All Rights Reserved.
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

#ifndef MACE_RUNTIMES_OPENCL_MTK_ION_OPENCL_MTK_ION_EXECUTOR_H_
#define MACE_RUNTIMES_OPENCL_MTK_ION_OPENCL_MTK_ION_EXECUTOR_H_

#include <memory>

#include "mace/runtimes/opencl/core/opencl_executor.h"

namespace mace {

class OpenclMtkIonExecutor : public OpenclExecutor {
 public:
  OpenclMtkIonExecutor();
  ~OpenclMtkIonExecutor() = default;

  static OpenclMtkIonExecutor *Get(OpenclExecutor *executor);

  IONType ion_type() const override;
};

}  // namespace mace

#endif // MACE_RUNTIMES_OPENCL_MTK_ION_OPENCL_MTK_ION_EXECUTOR_H_