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

#ifndef MACE_FLOWS_CPU_TRANSPOSE_CONST_H_
#define MACE_FLOWS_CPU_TRANSPOSE_CONST_H_

#include "mace/public/mace.h"
#include "mace/utils/thread_pool.h"

namespace mace {
class Workspace;
MaceStatus TransposeConstForCPU(
    mace::utils::ThreadPool *thread_pool,
    Workspace *ws,
    Runtime *runtime,
    NetDef *net_def);

}  // namespace mace

#endif  // MACE_FLOWS_CPU_TRANSPOSE_CONST_H_
