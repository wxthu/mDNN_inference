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

#include "mace/ops/opencl/image/lstm_cell.h"

#include "mace/runtimes/opencl/opencl_runtime.h"

namespace mace {
namespace ops {
namespace opencl {
namespace image {

MaceStatus LSTMCellKernel::Compute(
    OpContext *context,
    const Tensor *input,
    const Tensor *pre_output,
    const Tensor *weight,
    const Tensor *bias,
    const Tensor *pre_cell,
    Tensor *cell,
    Tensor *output) {
  MACE_CHECK(pre_output->dim_size() == 2 && pre_output->dim(1) % 4 == 0,
             "LSTM hidden units should be a multiple of 4");

  const index_t height = input->dim(0);
  const index_t width = input->dim(1);
  const index_t hidden_units = pre_output->dim(1);
  const index_t w_blocks = hidden_units >> 2;

  auto executor = OpenclRuntime::Get(context)->GetOpenclExecutor();
  MACE_OUT_OF_RANGE_DEFINITION;

  if (kernel_.get() == nullptr) {
    std::set<std::string> built_options;
    MACE_OUT_OF_RANGE_CONFIG;
    MACE_NON_UNIFORM_WG_CONFIG;
    std::string kernel_name = MACE_OBFUSCATE_SYMBOL("lstmcell");
    built_options.emplace("-Dlstmcell=" + kernel_name);
    built_options.emplace("-DDATA_TYPE=" + DtToCLDt(DT_FLOAT));
    built_options.emplace("-DCMD_DATA_TYPE=" + DtToCLCMDDt(DT_FLOAT));

    MACE_RETURN_IF_ERROR(executor->BuildKernel("lstmcell", kernel_name,
                                               built_options, &kernel_));

    kwg_size_ =
        static_cast<uint32_t>(executor->GetKernelMaxWorkGroupSize(kernel_));
  }

  const uint32_t gws[2] = {static_cast<uint32_t>(w_blocks),
                           static_cast<uint32_t>(height)};

  MACE_OUT_OF_RANGE_INIT(kernel_);
  if (IsResetArgsNeeded(context, input_shape_, input->shape())) {
    std::vector<index_t> output_shape_padded = {height, 1, 1, hidden_units};
    MACE_RETURN_IF_ERROR(output->Resize(output_shape_padded));
    output->Reshape(pre_output->shape());
    MACE_RETURN_IF_ERROR(cell->Resize(output_shape_padded));
    cell->Reshape(pre_cell->shape());

    uint32_t idx = 0;
    MACE_OUT_OF_RANGE_SET_ARGS(kernel_);
    MACE_SET_2D_GWS_ARGS(kernel_, gws);
    kernel_.setArg(idx++, *(input->memory<cl::Image>()));
    kernel_.setArg(idx++, *(pre_output->mutable_memory<cl::Image>()));
    kernel_.setArg(idx++, *(weight->memory<cl::Image>()));
    kernel_.setArg(idx++, *(bias->memory<cl::Image>()));
    kernel_.setArg(idx++, *(pre_cell->memory<cl::Image>()));
    kernel_.setArg(idx++, forget_bias_);
    kernel_.setArg(idx++, static_cast<int32_t>(width));
    kernel_.setArg(idx++, static_cast<int32_t>(hidden_units));
    kernel_.setArg(idx++, static_cast<int32_t>(RoundUpDiv4(width)));
    kernel_.setArg(idx++, *(cell->memory<cl::Image>()));
    kernel_.setArg(idx++, *(output->mutable_memory<cl::Image>()));

    input_shape_ = input->shape();
  }

  const std::vector<uint32_t> lws = {kwg_size_ / 16, 16, 0};
  std::string tuning_key =
      Concat("lstmcell_opencl_kernel", output->dim(0), output->dim(1));
  MACE_RETURN_IF_ERROR(TuningOrRun2DKernel(executor, kernel_, tuning_key,
                                           gws, lws, context->future(), context));
  MACE_OUT_OF_RANGE_VALIDATION;

  return MaceStatus::MACE_SUCCESS;
}

}  // namespace image
}  // namespace opencl
}  // namespace ops
}  // namespace mace
