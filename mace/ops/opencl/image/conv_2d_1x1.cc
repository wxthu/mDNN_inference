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

#include "mace/core/ops/op_context.h"
#include "mace/ops/common/activation_type.h"
#include "mace/ops/common/utils.h"
#include "mace/runtimes/opencl/core/opencl_executor.h"
#include "mace/runtimes/opencl/core/opencl_helper.h"
#include "mace/runtimes/opencl/opencl_runtime.h"

namespace mace {
namespace ops {
namespace opencl {
namespace image {

namespace {
// (inputs + weights + outputs) * array_size * sizeof(float)
const uint32_t kernel_cache_size = (4 + 4 + 4) * 4 * 4;
// TODO(liuqi): Fix the specific value.
const uint32_t lws_limit = 128;
std::vector<uint32_t> LocalWS(OpenclExecutor *executor,
                              const uint32_t *gws,
                              const uint32_t kwg_size) {
  std::vector<uint32_t> lws(4, 0);
  if (kwg_size == 0) {
    lws[0] = lws[1] = lws[2] = 1;
  } else {
    uint64_t
        cache_size = executor->device_global_mem_cache_size();
    uint32_t compute_units = executor->device_compute_units();
    const uint32_t base =
        std::max<uint32_t>(cache_size / kBaseGPUMemCacheSize, 1);
    lws[1] = std::min<uint32_t>(gws[1], kwg_size);
    if (lws[1] >= base) {
      lws[0] = std::min<uint32_t>(gws[0], base);
    } else if ((1 < lws[1] && lws[1] < base) && gws[0] >= lws_limit) {
      lws[0] = std::min<uint32_t>(gws[0], base);
    } else {
      lws[0] = gws[0] / 8;
      if (lws[0] < base) {
        lws[0] = std::max<uint32_t>(gws[0] / 4, base);
      }
    }
    lws[0] = std::min<uint32_t>(lws[0], kwg_size / lws[1]);
    const uint32_t lws_size = lws[0] * lws[1];
    lws[2] = std::min<uint32_t>(
        (cache_size / kernel_cache_size / lws_size / compute_units) * 8,
        gws[2]);
    if (lws[2] == 0) {
      lws[2] = std::min<uint32_t>(gws[2], base);
    }
    lws[2] = std::max<uint32_t>(std::min<uint32_t>(lws[2], kwg_size / lws_size),
                                1);
  }
  return lws;
}

}  // namespace

MaceStatus Conv2dK1x1(OpContext *context,
                      cl::Kernel *kernel,
                      const Tensor *input,
                      const Tensor *filter,
                      const Tensor *bias,
                      const int stride_h,
                      const int stride_w,
                      const int *padding,
                      const int *dilations,
                      const ActivationType activation,
                      const float relux_max_limit,
                      const float activation_coefficient,
                      std::vector<index_t> *prev_input_shape,
                      Tensor *output,
                      uint32_t *kwg_size) {
  MACE_UNUSED(padding);
  MACE_UNUSED(dilations);
  const index_t batch = output->dim(0);
  const index_t height = output->dim(1);
  const index_t width = output->dim(2);
  const index_t channels = output->dim(3);
  const index_t input_batch = input->dim(0);
  const index_t input_height = input->dim(1);
  const index_t input_width = input->dim(2);
  const index_t input_channels = input->dim(3);

  const index_t channel_blocks = RoundUpDiv4(channels);
  const index_t width_blocks = RoundUpDiv4(width);
  const index_t input_channel_blocks = RoundUpDiv4(input_channels);

  auto executor = OpenclRuntime::Get(context)->GetOpenclExecutor();
  MACE_OUT_OF_RANGE_DEFINITION;

  if (kernel->get() == nullptr) {
    MACE_CHECK(input_batch == batch);
    std::set<std::string> built_options;
    MACE_OUT_OF_RANGE_CONFIG;
    MACE_NON_UNIFORM_WG_CONFIG;
    std::string kernel_name = MACE_OBFUSCATE_SYMBOL("conv_2d_1x1");
    built_options.emplace("-Dconv_2d_1x1=" + kernel_name);
    built_options.emplace("-DDATA_TYPE=" + DtToCLDt(DT_FLOAT));
    built_options.emplace("-DCMD_DATA_TYPE=" + DtToCLCMDDt(DT_FLOAT));
    if (bias != nullptr) {
      built_options.emplace("-DBIAS");
    }
    common::utils::FillBuiltOptions(&built_options, activation);

    MACE_RETURN_IF_ERROR(executor->BuildKernel("conv_2d_1x1", kernel_name,
                                               built_options, kernel));

    *kwg_size =
        static_cast<uint32_t>(executor->GetKernelMaxWorkGroupSize(*kernel));
  }

  const uint32_t gws[3] = {static_cast<uint32_t>(channel_blocks),
                           static_cast<uint32_t>(width_blocks),
                           static_cast<uint32_t>(height * batch)};
  MACE_OUT_OF_RANGE_INIT(*kernel);

  // Support different input size
  if (IsResetArgsNeeded(context, *prev_input_shape, input->shape())) {
    uint32_t idx = 0;
    MACE_OUT_OF_RANGE_SET_ARGS(*kernel);
    MACE_SET_3D_GWS_ARGS(*kernel, gws);
    kernel->setArg(idx++, *(input->memory<cl::Image>()));
    kernel->setArg(idx++, *(filter->memory<cl::Image>()));
    if (bias != nullptr) {
      kernel->setArg(idx++, *(bias->memory<cl::Image>()));
    }
    kernel->setArg(idx++, *(output->mutable_memory<cl::Image>()));
    // FIXME handle flexable data type: half not supported
    kernel->setArg(idx++, relux_max_limit);
    kernel->setArg(idx++, activation_coefficient);
    kernel->setArg(idx++, static_cast<int>(input_height));
    kernel->setArg(idx++, static_cast<int>(input_width));
    kernel->setArg(idx++, static_cast<int>(input_channel_blocks));
    kernel->setArg(idx++, static_cast<int>(height));
    kernel->setArg(idx++, static_cast<int>(width));
    kernel->setArg(idx++, stride_h);
    kernel->setArg(idx++, stride_w);

    *prev_input_shape = input->shape();
  }

  std::vector<uint32_t> lws = LocalWS(executor, gws, *kwg_size);
  std::string tuning_key =
      Concat("conv2d_1x1_opencl_kernel", output->dim(0), output->dim(1),
             output->dim(2), output->dim(3));
  MACE_RETURN_IF_ERROR(TuningOrRun3DKernel(executor, *kernel, tuning_key,
                                           gws, lws, context->future(), context));
  MACE_OUT_OF_RANGE_VALIDATION;
  return MaceStatus::MACE_SUCCESS;
}

}  // namespace image
}  // namespace opencl
}  // namespace ops
}  // namespace mace
