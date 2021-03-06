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
#ifndef MACE_OPS_OPENCL_IMAGE_REDUCE_H_
#define MACE_OPS_OPENCL_IMAGE_REDUCE_H_

#include "mace/ops/opencl/reduce.h"

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "mace/core/ops/op_context.h"
#include "mace/core/tensor.h"
#include "mace/runtimes/opencl/core/opencl_helper.h"
#include "mace/ops/common/reduce_type.h"

namespace mace {
namespace ops {
namespace opencl {
namespace image {

class ReduceKernel : public OpenCLReduceKernel {
 public:
  ReduceKernel(ReduceType type,
               const std::vector<int> &axis)
      : reduce_type_(type), axis_(axis) {}

  MaceStatus Compute(
      OpContext *context,
      const Tensor *input,
      Tensor *output) override;

 private:
  MaceStatus BuildReduceKernel(OpenclExecutor *executor,
                               bool divisable_by_four = false);
  MaceStatus GraduallyComputeReduceHW(
      OpContext *context, const index_t batch, const index_t channel_blocks,
      const index_t in_height, const index_t in_width,
      const index_t out_height, const index_t out_width,
      const index_t org_height, const index_t org_width,
      const cl::Image *input, cl::Image *output,
      std::vector<StatsFuture> *futures);
  MaceStatus GraduallyComputeReduceC(
      OpContext *context,
      const index_t batch,
      const index_t height,
      const index_t width,
      const index_t channels,
      const index_t channel_blocks,
      const index_t out_ch_blks,
      const index_t in_ch_blks,
      const cl::Image *input, cl::Image *output,
      std::vector<StatsFuture> *futures);
  MaceStatus ReduceHW(OpContext *context,
                      const Tensor *input,
                      Tensor *output);
  MaceStatus ReduceC(OpContext *context,
                      const Tensor *input,
                      Tensor *output);

 private:
  ReduceType reduce_type_;
  std::vector<int> axis_;
  cl::Kernel kernel_;
  uint32_t kwg_size_;
  std::vector<index_t> input_shape_;
};

}  // namespace image
}  // namespace opencl
}  // namespace ops
}  // namespace mace

#endif  // MACE_OPS_OPENCL_IMAGE_REDUCE_H_
