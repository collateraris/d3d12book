#pragma once

#include <URootObject.h>
#include <CommandList.h>
#include <RootSignature.h>
#include <DescriptorAllocation.h>
#include <ShaderCommonInclude.h>
#include <StructuredBuffer.h>
#include <Texture.h>
#include <GeometryPrimitive.h>
#include <Light.h>

#include <memory>
#include <vector>
#include <bitset>

namespace dx12demo::core
{
    class ComputePerformanceTest : URootObject
    {
    public:

        ComputePerformanceTest();

        virtual ~ComputePerformanceTest();

        void InitBuffers(std::shared_ptr<CommandList>& commandList);

        void StartCompute(std::shared_ptr<CommandList>& commandList);

        void Compute(std::shared_ptr<CommandList>& commandList);

    private:

        RootSignature m_RootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;

        StructuredBuffer m_InputArrBuffer;
        StructuredBuffer m_OutputParBuffer;
    };
}