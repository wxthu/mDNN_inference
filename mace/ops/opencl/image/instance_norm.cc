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

#include "mace/ops/common/utils.h"
#include "mace/ops/opencl/image/instance_norm.h"

#include "mace/runtimes/opencl/opencl_runtime.h"

namespace mace {
namespace ops {
namespace opencl {
namespace image {

InstanceNormKernel::InstanceNormKernel(const float epsilon,
                                       const ActivationType activation,
                                       const float relux_max_limit,
                                       const float activation_coefficient,
                                       const bool affine)
    : epsilon_(epsilon),
      activation_(activation),
      relux_max_limit_(relux_max_limit),
      activation_coefficient_(activation_coefficient),
      affine_(affine) {}

MaceStatus InstanceNormKernel::Compute(
    OpContext *context,
    const Tensor *input,
    const Tensor *scale,
    const Tensor *offset,
    const Tensor *mean,
    const Tensor *var,
    Tensor *output) {
  const index_t batch = input->dim(0);
  const index_t height = input->dim(1);
  const index_t width = input->dim(2);
  const index_t channels = input->dim(3);

  const index_t channel_blocks = RoundUpDiv4(channels);

  const uint32_t gws[3] = {static_cast<uint32_t>(channel_blocks),
                           static_cast<uint32_t>(width),
                           static_cast<uint32_t>(height * batch)};
  auto executor = OpenclRuntime::Get(context)->GetOpenclExecutor();
  MACE_OUT_OF_RANGE_DEFINITION;

  if (kernel_.get() == nullptr) {
    std::set<std::string> built_options;
    MACE_OUT_OF_RANGE_CONFIG;
    MACE_NON_UNIFORM_WG_CONFIG;
    std::string kernel_name = MACE_OBFUSCATE_SYMBOL("instance_norm");
    built_options.emplace("-Dinstance_norm=" + kernel_name);
    built_options.emplace("-DDATA_TYPE=" + DtToCLDt(DT_FLOAT));
    built_options.emplace("-DCMD_DATA_TYPE=" + DtToCLCMDDt(DT_FLOAT));
    if (affine_) {
      built_options.emplace("-DIN_AFFINE");
      MACE_CHECK((scale != nullptr) && (offset != nullptr),
                 "When affine is true, scale and offset must not be null");
    }
    common::utils::FillBuiltOptions(&built_options, activation_);

    MACE_RETURN_IF_ERROR(executor->BuildKernel("instance_norm", kernel_name,
                                               built_options, &kernel_));

    kwg_size_ =
        static_cast<uint32_t>(executor->GetKernelMaxWorkGroupSize(kernel_));
  }
  MACE_OUT_OF_RANGE_INIT(kernel_);
  if (IsResetArgsNeeded(context, input_shape_, input->shape())) {
    uint32_t idx = 0;
    MACE_OUT_OF_RANGE_SET_ARGS(kernel_);
    MACE_SET_3D_GWS_ARGS(kernel_, gws);
    kernel_.setArg(idx++, *(input->memory<cl::Image>()));
    if (affine_) {
      kernel_.setArg(idx++, *(scale->memory<cl::Image>()));
      kernel_.setArg(idx++, *(offset->memory<cl::Image>()));
    }
    kernel_.setArg(idx++, *(mean->memory<cl::Image>()));
    kernel_.setArg(idx++, *(var->memory<cl::Image>()));
    kernel_.setArg(idx++, epsilon_);
    kernel_.setArg(idx++, *(output->mutable_memory<cl::Image>()));
    kernel_.setArg(idx++, relux_max_limit_);
    kernel_.setArg(idx++, activation_coefficient_);
    kernel_.setArg(idx++, static_cast<int>(height));

    input_shape_ = input->shape();
  }

  const std::vector<uint32_t> lws = Default3DLocalWS(executor, gws, kwg_size_);
  std::string tuning_key =
      Concat("instance_norm_opencl_kernel", affine_, activation_,
             output->dim(0), output->dim(1), output->dim(2), output->dim(3));
  MACE_RETURN_IF_ERROR(TuningOrRun3DKernel(executor, kernel_, tuning_key,
                                           gws, lws, context->future(), context));
  MACE_OUT_OF_RANGE_VALIDATION;
  return MaceStatus::MACE_SUCCESS;
}

}  // namespace image
}  // namespace opencl
}  // namespace ops
}  // namespace mace
