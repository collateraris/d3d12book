#include <LightCulling.h>

#include <DX12LibPCH.h>
#include <Application.h>
#include <CommandQueue.h>
#include <LightCulling_CS.h>

using namespace dx12demo::core;

namespace ComputeParams
{
    enum
    {
        b0ScreenToViewParamsCB,
        b1DispatchParamsCB,
        b2NumLightsCB,
        t0DepthTextureVSTex,
        t1FrustumsSB,
        t2LightCountHeatMapTex,
        t3LightsSB,
        u0DebugTextureTex,
        u1OpaqueLightIndexCounterSB,
        u2TransparentLightIndexCounterSB,
        u3OpaqueLightIndexListSB,
        u4TransparentLightIndexListSB,
        u5OpaqueLightGridTex,
        u6TransparentLightGridTex,
        NumRootParameters
    };
}

LightCulling::LightCulling()
{
    auto device = GetApp().GetDevice();

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    CD3DX12_ROOT_PARAMETER1 rootParameters[ComputeParams::NumRootParameters];
    rootParameters[ComputeParams::b0ScreenToViewParamsCB].InitAsConstantBufferView(0, 0);
    rootParameters[ComputeParams::b1DispatchParamsCB].InitAsConstantBufferView(1, 0);
    rootParameters[ComputeParams::b2NumLightsCB].InitAsConstantBufferView(2, 0);

    CD3DX12_DESCRIPTOR_RANGE1 depthTexVSDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    rootParameters[ComputeParams::t0DepthTextureVSTex].InitAsDescriptorTable(1, &depthTexVSDescrRange);
    CD3DX12_DESCRIPTOR_RANGE1 gridFrustumDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
    rootParameters[ComputeParams::t1FrustumsSB].InitAsDescriptorTable(1, &gridFrustumDescrRange);
    CD3DX12_DESCRIPTOR_RANGE1 lightCountHeatMapDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);
    rootParameters[ComputeParams::t2LightCountHeatMapTex].InitAsDescriptorTable(1, &lightCountHeatMapDescrRange);
    CD3DX12_DESCRIPTOR_RANGE1 lightsDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3);
    rootParameters[ComputeParams::t3LightsSB].InitAsDescriptorTable(1, &lightsDescrRange);

    CD3DX12_DESCRIPTOR_RANGE1 debugTexDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
    rootParameters[ComputeParams::u0DebugTextureTex].InitAsDescriptorTable(1, &debugTexDescrRange);
    CD3DX12_DESCRIPTOR_RANGE1 opaqueLightIndexCounterDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);
    rootParameters[ComputeParams::u1OpaqueLightIndexCounterSB].InitAsDescriptorTable(1, &opaqueLightIndexCounterDescrRange);
    CD3DX12_DESCRIPTOR_RANGE1 transparentLightIndexCounterDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 2);
    rootParameters[ComputeParams::u2TransparentLightIndexCounterSB].InitAsDescriptorTable(1, &transparentLightIndexCounterDescrRange);
    CD3DX12_DESCRIPTOR_RANGE1 opaqueLightIndexListDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 3);
    rootParameters[ComputeParams::u3OpaqueLightIndexListSB].InitAsDescriptorTable(1, &opaqueLightIndexListDescrRange);
    CD3DX12_DESCRIPTOR_RANGE1 transparentLightIndexListDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 4);
    rootParameters[ComputeParams::u4TransparentLightIndexListSB].InitAsDescriptorTable(1, &transparentLightIndexListDescrRange);
    CD3DX12_DESCRIPTOR_RANGE1 opaqueLightGridTexDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 5);
    rootParameters[ComputeParams::u5OpaqueLightGridTex].InitAsDescriptorTable(1, &opaqueLightGridTexDescrRange);
    CD3DX12_DESCRIPTOR_RANGE1 transparentLightGridTexDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 6);
    rootParameters[ComputeParams::u6TransparentLightGridTex].InitAsDescriptorTable(1, &transparentLightGridTexDescrRange);

    const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
        0, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(ComputeParams::NumRootParameters, rootParameters, 1, &linearClamp, D3D12_ROOT_SIGNATURE_FLAG_NONE);

    m_RootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

    // Create the PSO for GenerateMips shader.
    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_CS CS;
    } pipelineStateStream;

    pipelineStateStream.pRootSignature = m_RootSignature.GetRootSignature().Get();
    pipelineStateStream.CS = { g_LightCulling_CS, sizeof(g_LightCulling_CS) };

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
       sizeof(PipelineStateStream), &pipelineStateStream
    };

    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState)));

    auto commandQueue = GetApp().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    auto commandList = commandQueue->GetCommandList();
    commandList->LoadTextureFromFile(m_LightCullingHeatMap, L"Assets/Textures/LightCountHeatMap.png");

    // Light list index counter (initial value buffer - required to reset the light list index counters back to 0)
    std::vector<uint32_t> lightListIndexCounterInitialValue = { 0 };
    commandList->CopyStructuredBuffer(m_LightListIndexCounterOpaqueBuffer, lightListIndexCounterInitialValue);
    commandList->CopyStructuredBuffer(m_LightListIndexCounterTransparentBuffer, lightListIndexCounterInitialValue);

    auto fenceValue = commandQueue->ExecuteCommandList(commandList);
    commandQueue->WaitForFenceValue(fenceValue);
}

LightCulling::~LightCulling()
{

}

void LightCulling::StartCompute(std::shared_ptr<CommandList>& commandList)
{
    commandList->SetComputeRootSignature(m_RootSignature);
    commandList->SetPipelineState(m_PipelineState);

    m_CurrState.set(EBitsetStates::bStartCompute);
}

void LightCulling::InitDebugTex(int screenW, int screenH)
{
    DXGI_FORMAT debugTexFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
    auto debugTexDesc = CD3DX12_RESOURCE_DESC::Tex2D(debugTexFormat, screenW, screenH);
    debugTexDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    m_DebugTex = core::Texture(debugTexDesc, nullptr,
        TextureUsage::Albedo,
        L"Debug Tex for light culling");

    m_CurrState.set(EBitsetStates::bInitDebugTex);
}

void LightCulling::InitLightIndexListBuffers(std::shared_ptr<CommandList>& commandList, const DispatchParams& dispatchPar, uint32_t average_overlapping_lights_per_tile)
{
    const auto& numThreads = dispatchPar.m_NumThreads;
    auto lightsNum = numThreads.x * numThreads.y * numThreads.z * average_overlapping_lights_per_tile;
    std::vector<uint32_t> bufferSize(lightsNum, 0);
    commandList->CopyStructuredBuffer(m_LightIndexListOpaqueBuffer, bufferSize);
    commandList->CopyStructuredBuffer(m_LightIndexListTransparentBuffer, bufferSize);

    m_CurrState.set(EBitsetStates::bInitLightIndexBuffer);
}

void LightCulling::InitLightGridTexture(const DispatchParams& dispatchPar)
{
    DXGI_FORMAT texFormat = DXGI_FORMAT_R32G32_UINT;
    auto texDesc = CD3DX12_RESOURCE_DESC::Tex2D(texFormat, dispatchPar.m_NumThreads.x, dispatchPar.m_NumThreads.y);
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    m_OpaqueLightGrid = core::Texture(texDesc, nullptr,
        TextureUsage::Albedo,
        L"OpaqueLightGrid tex for light culling");
    m_TransparentLightGrid = core::Texture(texDesc, nullptr,
        TextureUsage::Albedo,
        L"TransparentLightGrid tex for light culling");

    m_CurrState.set(EBitsetStates::bInitLightGridTex);
}


void LightCulling::AttachLightsInBuffer(std::shared_ptr<CommandList>& commandList, const std::vector<Light>& lights)
{
    commandList->CopyStructuredBuffer(m_LightsBuffer, lights);
    m_NumLights.NUM_LIGHTS = lights.size();
    m_CurrState.set(EBitsetStates::bAttachLightsInBuffer);
}

void LightCulling::AttachDepthTex(std::shared_ptr<CommandList>& commandList, const Texture& depthTex, const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc/* = nullptr*/)
{
    assert(m_CurrState.test(EBitsetStates::bStartCompute));
    
    commandList->TransitionBarrier(depthTex, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    commandList->SetShaderResourceView(ComputeParams::t0DepthTextureVSTex, 0, depthTex,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        0, 0, srvDesc);
    m_CurrState.set(EBitsetStates::bAttachDepthTex);
}

void LightCulling::AttachGridViewFrustums(std::shared_ptr<CommandList>& commandList, const StructuredBuffer& frustums)
{
    assert(m_CurrState.test(EBitsetStates::bStartCompute));

    commandList->SetShaderResourceView(ComputeParams::t1FrustumsSB, 0, frustums);
    m_CurrState.set(EBitsetStates::bAttachGridViewFrustums);
}

void LightCulling::Compute(std::shared_ptr<CommandList>& commandList, const ScreenToViewParams& params, const DispatchParams& dispatchPar)
{
    assert(m_CurrState.test(EBitsetStates::bStartCompute));
    assert(m_CurrState.test(EBitsetStates::bAttachDepthTex));
    assert(m_CurrState.test(EBitsetStates::bAttachLightsInBuffer));
    assert(m_CurrState.test(EBitsetStates::bAttachGridViewFrustums));
    assert(m_CurrState.test(EBitsetStates::bInitDebugTex));
    assert(m_CurrState.test(EBitsetStates::bInitLightGridTex));
    assert(m_CurrState.test(EBitsetStates::bInitLightIndexBuffer));

    m_CurrState.reset(EBitsetStates::bStartCompute);
    m_CurrState.reset(EBitsetStates::bAttachDepthTex);
    m_CurrState.reset(EBitsetStates::bAttachLightsInBuffer);

    commandList->SetComputeDynamicConstantBuffer(ComputeParams::b0ScreenToViewParamsCB, params);
    commandList->SetComputeDynamicConstantBuffer(ComputeParams::b1DispatchParamsCB, dispatchPar);
    commandList->SetComputeDynamicConstantBuffer(ComputeParams::b2NumLightsCB, m_NumLights);

    commandList->SetShaderResourceView(ComputeParams::t2LightCountHeatMapTex, 0, m_LightCullingHeatMap);
    commandList->SetShaderResourceView(ComputeParams::t3LightsSB, 0, m_LightsBuffer);

    commandList->SetUnorderedAccessView(ComputeParams::u0DebugTextureTex, 0, m_DebugTex);
    commandList->SetUnorderedAccessView(ComputeParams::u1OpaqueLightIndexCounterSB, 0, m_LightListIndexCounterOpaqueBuffer);
    commandList->SetUnorderedAccessView(ComputeParams::u2TransparentLightIndexCounterSB, 0, m_LightListIndexCounterTransparentBuffer);
    commandList->SetUnorderedAccessView(ComputeParams::u3OpaqueLightIndexListSB, 0, m_LightIndexListOpaqueBuffer);
    commandList->SetUnorderedAccessView(ComputeParams::u4TransparentLightIndexListSB, 0, m_LightIndexListTransparentBuffer);
    commandList->SetUnorderedAccessView(ComputeParams::u5OpaqueLightGridTex, 0, m_OpaqueLightGrid);
    commandList->SetUnorderedAccessView(ComputeParams::u6TransparentLightGridTex, 0, m_TransparentLightGrid);

    const auto& numThreads = dispatchPar.m_NumThreads;
    commandList->Dispatch(numThreads.x, numThreads.y, numThreads.z);
}

const Texture& LightCulling::GetDebugTex() const
{
    return m_DebugTex;
}

const Texture& LightCulling::GetOpaqueLightGrid() const
{
    return m_OpaqueLightGrid;
}

const StructuredBuffer& LightCulling::GetOpaqueLightIndexList() const
{
    return m_LightIndexListOpaqueBuffer;
}

const StructuredBuffer& LightCulling::GetLightsBuffer() const
{
    return m_LightsBuffer;
}