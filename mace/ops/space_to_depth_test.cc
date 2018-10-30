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

#include <fstream>

#include <vector>
#include "mace/core/op_def_registry.h"
#include "mace/ops/ops_test_util.h"

namespace mace {
namespace ops {
namespace test {

namespace {
template <DeviceType D>
void RunSpaceToDepth(const std::vector<index_t> &input_shape,
                     const std::vector<float> &input_data,
                     const int block_size,
                     const std::vector<index_t> &expected_shape,
                     const std::vector<float> &expected_data) {
  OpsTestNet net;
  net.AddInputFromArray<D, float>("Input", input_shape, input_data);
  // Construct graph
  if (D == DeviceType::CPU) {
    net.TransformDataFormat<DeviceType::CPU, float>("Input", NHWC, "InputNCHW",
                                                    NCHW);
    OpDefBuilder("SpaceToDepth", "SpaceToDepthTest")
        .Input("InputNCHW")
        .Output("OutputNCHW")
        .AddIntArg("block_size", block_size)
        .Finalize(net.NewOperatorDef());
    // Run
    net.RunOp(D);
    net.TransformDataFormat<DeviceType::CPU, float>("OutputNCHW", NCHW,
                                                    "Output", NHWC);

  } else {
    BufferToImage<D, float>(&net, "Input", "InputImage",
                            kernels::BufferType::IN_OUT_CHANNEL);
    OpDefBuilder("SpaceToDepth", "SpaceToDepthTest")
        .Input("InputImage")
        .Output("OutputImage")
        .AddIntArg("block_size", block_size)
        .Finalize(net.NewOperatorDef());
    // Run
    net.RunOp(D);
  }

  if (D == DeviceType::GPU) {
    ImageToBuffer<DeviceType::GPU, float>(&net, "OutputImage", "Output",
                                          kernels::BufferType::IN_OUT_CHANNEL);
  }
  auto expected = net.CreateTensor<float>(expected_shape, expected_data);
  ExpectTensorNear<float>(*expected, *net.GetOutput("Output"), 1e-5);
}
}  // namespace

class SpaceToDepthOpTest : public OpsTestBase {};

TEST_F(SpaceToDepthOpTest, Input2x4x4_B2_CPU) {
  RunSpaceToDepth<DeviceType::CPU>(
      {1, 2, 4, 4},
      {0, 1, 2,  3,  4,  5,  6,  7,  16, 17, 18, 19, 20, 21, 22, 23,
       8, 9, 10, 11, 12, 13, 14, 15, 24, 25, 26, 27, 28, 29, 30, 31},
      2, {1, 1, 2, 16},
      {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
       16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31});
}

TEST_F(SpaceToDepthOpTest, Input2x4x4_B2_OPENCL) {
  RunSpaceToDepth<DeviceType::GPU>(
      {1, 2, 4, 4},
      {0, 1, 2,  3,  4,  5,  6,  7,  16, 17, 18, 19, 20, 21, 22, 23,
       8, 9, 10, 11, 12, 13, 14, 15, 24, 25, 26, 27, 28, 29, 30, 31},
      2, {1, 1, 2, 16},
      {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
       16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31});
}

TEST_F(SpaceToDepthOpTest, Input2x2x4_B2_CPU) {
  RunSpaceToDepth<DeviceType::CPU>(
      {1, 2, 2, 4},
      {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}, 2, {1, 1, 1, 16},
      {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16});
}

TEST_F(SpaceToDepthOpTest, Input4x4x1_B2_OPENCL) {
  RunSpaceToDepth<DeviceType::GPU>(
      {1, 2, 2, 4},
      {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}, 2, {1, 1, 1, 16},
      {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16});
}

namespace {
template <DeviceType D, typename T>
void RandomTest(const int block_size,
                const std::vector<index_t> &shape) {
  testing::internal::LogToStderr();
  srand(time(NULL));

  // Construct graph
  OpsTestNet net;

  // Add input data
  net.AddRandomInput<D, float>("Input", shape);
  net.TransformDataFormat<DeviceType::CPU, float>("Input", NHWC, "InputNCHW",
                                                  NCHW);
  OpDefBuilder("SpaceToDepth", "SpaceToDepthTest")
      .Input("InputNCHW")
      .AddIntArg("block_size", block_size)
      .Output("OutputNCHW")
      .Finalize(net.NewOperatorDef());

  // Run
  net.RunOp();

  net.TransformDataFormat<DeviceType::CPU, float>("OutputNCHW", NCHW, "Output",
                                                  NHWC);

  BufferToImage<D, T>(&net, "Input", "InputImg",
                      kernels::BufferType::IN_OUT_CHANNEL);

  OpDefBuilder("SpaceToDepth", "SpaceToDepthTest")
      .Input("InputImg")
      .AddIntArg("block_size", block_size)
      .AddIntArg("T", static_cast<int>(DataTypeToEnum<T>::value))
      .Output("OutputImg")
      .Finalize(net.NewOperatorDef());

  // Run
  net.RunOp(D);

  ImageToBuffer<D, float>(&net, "OutputImg", "OPENCLOutput",
                          kernels::BufferType::IN_OUT_CHANNEL);

  if (DataTypeToEnum<T>::value == DT_FLOAT) {
    ExpectTensorNear<float>(*net.GetTensor("Output"),
                            *net.GetOutput("OPENCLOutput"), 1e-5);
  } else {
    ExpectTensorNear<float>(*net.GetTensor("Output"),
                            *net.GetOutput("OPENCLOutput"), 1e-3, 1e-4);
  }
}
}  // namespace

TEST_F(SpaceToDepthOpTest, OPENCLRandomFloat) {
  RandomTest<DeviceType::GPU, float>(2, {1, 384, 384, 32});
}

TEST_F(SpaceToDepthOpTest, OPENCLRandomHalf) {
  RandomTest<DeviceType::GPU, half>(2, {1, 384, 384, 32});
}

}  // namespace test
}  // namespace ops
}  // namespace mace
