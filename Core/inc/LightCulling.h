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
    class LightCulling : URootObject
    {
    public:

        LightCulling();

        virtual ~LightCulling();

        void InitDebugTex(int screenW, int screenH);

        void InitLightIndexListBuffers(std::shared_ptr<CommandList>& commandList, const DispatchParams& dispatchPar, uint32_t average_overlapping_lights_per_tile);

        void InitLightGridTexture(const DispatchParams& dispatchPar);

        void StartCompute(std::shared_ptr<CommandList>& commandList);

        void AttachLightsBuffer(std::shared_ptr<CommandList>& commandList, const StructuredBuffer& lights, const StructuredBuffer& lightsNum);

        void AttachDepthTex(std::shared_ptr<CommandList>& commandList, const Texture& depthTex, const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr);

        void AttachGridViewFrustums(std::shared_ptr<CommandList>& commandList, const StructuredBuffer& frustums);

        void Compute(std::shared_ptr<CommandList>& commandList, const ScreenToViewParams& params, const DispatchParams& dispatchPar);

        const Texture& GetDebugTex() const;

        const Texture& GetOpaqueLightGrid() const;

        const StructuredBuffer& GetOpaqueLightIndexList() const;

    private:

        struct LightInfo
        {
            int NUM_LIGHTS;
        };

        //state
        enum EBitsetStates
        {
            bStartCompute = 1,
            bInitDebugTex,
            bInitLightIndexBuffer,
            bInitLightGridTex,
            bAttachDepthTex,
            bAttachGridViewFrustums
        };

        RootSignature m_RootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;

        Texture m_DebugTex;
        Texture m_OpaqueLightGrid;
        Texture m_TransparentLightGrid;
        Texture m_LightCullingHeatMap;

        StructuredBuffer m_LightListIndexCounterOpaqueBuffer;
        StructuredBuffer m_LightListIndexCounterTransparentBuffer;
        StructuredBuffer m_LightIndexListOpaqueBuffer;
        StructuredBuffer m_LightIndexListTransparentBuffer;

        std::bitset<9> m_CurrState;
    };
}