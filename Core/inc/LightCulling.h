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

        void InitLightIndexListBuffers(const DispatchParams& dispatchPar, uint32_t average_overlapping_lights_per_tile);

        void InitLightGridTexture(const DispatchParams& dispatchPar);

        void InitLightsBuffer(const std::vector<Light>& lights);

        void StartCompute();

        void AttachNumLights(int num);

        void AttachDepthTex(const Texture& depthTex);

        void AttachGridViewFrustums(const StructuredBuffer& frustums);

        void Compute(const ScreenToViewParams& params, const DispatchParams& dispatchPar);

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
            bInitLightsBuffer,
            bAttachNumLights,
            bAttachDepthTex,
            bAttachGridViewFrustums
        };

        std::shared_ptr<CommandList> m_CurrCommandList;
        bool bStartComputeInit = false;

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
        StructuredBuffer m_LightsBuffer;

        LightCulling::LightInfo m_NumLights;

        std::bitset<9> m_CurrState;
    };
}