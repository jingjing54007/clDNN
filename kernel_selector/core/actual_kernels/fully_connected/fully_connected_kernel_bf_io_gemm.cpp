﻿/*
// Copyright (c) 2016 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include "fully_connected_kernel_bf_io_gemm.h"
#include "kernel_selector_utils.h"

namespace KernelSelector {

    ParamsKey FullyConnected_bf_io_GEMM::GetSupportedKey() const
    {
        ParamsKey k;
        k.EnableInputDataType(Datatype::F16);
        k.EnableInputDataType(Datatype::F32);
        k.EnableOutputDataType(Datatype::F16);
        k.EnableOutputDataType(Datatype::F32);
        k.EnableInputWeightsType(WeightsType::F16);
        k.EnableInputWeightsType(WeightsType::F32);
        k.EnableAllInputLayout();
        k.EnableOutputLayout(DataLayout::bf);
        k.EnableBiasPerOutput();
        k.EnableBiasPerFeature();
        k.EnableNonBiasTerm();
        k.EnableTensorOffset();
        k.EnableTensorPitches();
        k.EnableBatching();
        return k;
    }

    std::unique_ptr<FullyConnected_bf_io_GEMM::Parent::DispatchData> FullyConnected_bf_io_GEMM::SetDefault(const FullyConnectedParams& params) const
    {
        auto runInfo = Parent::SetDefault(params);

        const uint32_t localWorkSizeX = 64;
        const uint32_t globalWorkSizeX = localWorkSizeX;

        std::vector<size_t> global = { globalWorkSizeX, params.output.Feature().v, params.output.Batch().v };
        std::vector<size_t> local = { localWorkSizeX, 1, 1 };

        runInfo->gws0 = global[0];
        runInfo->gws1 = global[1];
        runInfo->gws2 = 1;

        runInfo->lws0 = local[0];
        runInfo->lws1 = local[1];
        runInfo->lws2 = 1;

        runInfo->effiency = FORCE_PRIORITY_6;

        return std::move(runInfo);
    }

    JitConstants FullyConnected_bf_io_GEMM::GetJitConstants(const FullyConnectedParams& params, const DispatchData& kd) const
    {
        auto jit = Parent::GetJitConstants(params, kd);

        if (params.inputs[0].GetDType() == Datatype::F16)
        {
            jit.AddConstant(MakeJitConstant("__fc_f16", ""));
        }
        else
        {
            jit.AddConstant(MakeJitConstant("__fc_f32", ""));
        }

        const uint32_t localWorkSizeX = 64;
        const uint32_t globalWorkSizeX = localWorkSizeX;
        const uint32_t vecSize = 4;
        size_t matrixLineSize = params.inputs[0].Batch().pitch;

        jit.AddConstants({
            MakeJitConstant("LAST_INPUT_SIZE_REMAINDER", matrixLineSize % (globalWorkSizeX * vecSize)),
            MakeJitConstant("LAST_INPUT_SIZE_DIV_4", matrixLineSize % vecSize),
        });

        return jit;
    }

    KernelsData FullyConnected_bf_io_GEMM::GetKernelsData(const Params& params, const OptionalParams& options) const
    {
        return GetCommonKernelsData(params, options, DataLayout::bf, { WeightsLayout::oiyx }, FORCE_PRIORITY_6);
    }
}
