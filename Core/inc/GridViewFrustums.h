#pragma once

#include <URootObject.h>
#include <CommandList.h>
#include <RootSignature.h>
#include <DescriptorAllocation.h>
#include <ShaderCommonInclude.h>
#include <StructuredBuffer.h>

namespace dx12demo::core
{
	class GridViewFrustum : URootObject
	{
    public:

        GridViewFrustum();

        virtual ~GridViewFrustum();

        void Compute(const ScreenToViewParams& params, const DispatchParams& dispatchPar);

        const StructuredBuffer& GetGridFrustums() const;

    private:

        RootSignature m_RootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;

        std::vector<FrustumPrimitive> m_GridFrustums;
        StructuredBuffer m_GridFrustumBuffer;
	};
}