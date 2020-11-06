#pragma once

#include <URootObject.h>
#include <CommandList.h>
#include <RootSignature.h>
#include <StructuredBuffer.h>
#include <DirectXMath.h>
#include <Light.h>

#include <memory>
#include <vector>

namespace dx12demo::core
{
    class LightsToView : URootObject
    {
    public:

        LightsToView();

        virtual ~LightsToView();

        void InitLightsBuffer(std::shared_ptr<CommandList>& commandList, const std::vector<Light>& lights);

        void StartCompute(std::shared_ptr<CommandList>& commandList);

        void Compute(std::shared_ptr<CommandList>& commandList, const DirectX::XMMATRIX& modelViewMatrix);

        int GetNumLights() const;

        const StructuredBuffer& GetNumLightsBuffer() const;

        const StructuredBuffer& GetLightsBuffer() const;

    private:


        struct LightInfo
        {
            int NUM_LIGHTS;
        };

        struct ModelViewMatrix
        {
            DirectX::XMMATRIX matrix;
        };

        RootSignature m_RootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;

        StructuredBuffer m_LightsBuffer;
        StructuredBuffer m_NumLightsBuffer;

        LightsToView::LightInfo m_NumLights;
        LightsToView::ModelViewMatrix m_View;
    };
}