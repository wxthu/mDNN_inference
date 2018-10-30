// Copyright 2018 Xiaomi, Inc.  All rights reserved.
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

#ifndef MACE_KERNELS_OPENCL_FULLY_CONNECTED_H_
#define MACE_KERNELS_OPENCL_FULLY_CONNECTED_H_

#include "mace/kernels/activation.h"

#include "mace/public/mace.h"
#include "mace/utils/utils.h"

namespace mace {

class OpContext;
class Tensor;

namespace kernels {
class OpenCLFullyConnectedKernel {
 public:
  virtual MaceStatus Compute(
      OpContext *context,
      const Tensor *input,
      const Tensor *weight,
      const Tensor *bias,
      const ActivationType activation,
      const float relux_max_limit,
      Tensor *output) = 0;
  MACE_EMPTY_VIRTUAL_DESTRUCTOR(OpenCLFullyConnectedKernel);
};

}  // namespace kernels
}  // namespace mace

#endif  // MACE_KERNELS_OPENCL_FULLY_CONNECTED_H_
