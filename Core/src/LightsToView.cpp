#include <LightsToView.h>

#include <DX12LibPCH.h>
#include <Application.h>
#include <CommandQueue.h>
#include <LightsToView_CS.h>

using namespace dx12demo::core;

namespace ComputeParams
{
    enum
    {
        b0MatCB,
        t0NumLightSB,
        u0LightsSB,
        NumRootParameters
    };
}

LightsToView::LightsToView()
{
    auto device = GetApp().GetDevice();

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    CD3DX12_ROOT_PARAMETER1 rootParameters[ComputeParams::NumRootParameters];
    rootParameters[ComputeParams::b0MatCB].InitAsConstantBufferView(0, 0);
    CD3DX12_DESCRIPTOR_RANGE1 numLigthDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    rootParameters[ComputeParams::t0NumLightSB].InitAsDescriptorTable(1, &numLigthDescrRange);
    CD3DX12_DESCRIPTOR_RANGE1 lightBufferDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
    rootParameters[ComputeParams::u0LightsSB].InitAsDescriptorTable(1, &lightBufferDescrRange);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(ComputeParams::NumRootParameters, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

    m_RootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

    // Create the PSO for GenerateMips shader.
    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_CS CS;
    } pipelineStateStream;

    pipelineStateStream.pRootSignature = m_RootSignature.GetRootSignature().Get();
    pipelineStateStream.CS = { g_LightsToView_CS, sizeof(g_LightsToView_CS) };

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
       sizeof(PipelineStateStream), &pipelineStateStream
    };

    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState)));
}

LightsToView::~LightsToView()
{

}

void LightsToView::InitLightsBuffer(std::shared_ptr<CommandList>& commandList, const std::vector<Light>& lights)
{
    commandList->CopyStructuredBuffer(m_LightsBuffer, lights);
    m_NumLights.NUM_LIGHTS = lights.size();
    std::vector<LightInfo> numLight = { m_NumLights };
    commandList->CopyStructuredBuffer(m_NumLightsBuffer, numLight);
}

void LightsToView::StartCompute(std::shared_ptr<CommandList>& commandList)
{
    commandList->SetComputeRootSignature(m_RootSignature);
    commandList->SetPipelineState(m_PipelineState);
}

void LightsToView::Compute(std::shared_ptr<CommandList>& commandList, const DirectX::XMMATRIX& viewMatrix)
{
    m_View.matrix = viewMatrix;
    commandList->SetComputeDynamicConstantBuffer(ComputeParams::b0MatCB, m_View);
    commandList->SetShaderResourceView(ComputeParams::t0NumLightSB, 0, m_NumLightsBuffer);
    commandList->SetUnorderedAccessView(ComputeParams::u0LightsSB, 0, m_LightsBuffer);

    commandList->Dispatch(1, 1, 1); 
}

int LightsToView::GetNumLights() const
{
    return m_NumLights.NUM_LIGHTS;
}

const StructuredBuffer& LightsToView::GetNumLightsBuffer() const
{
    return m_NumLightsBuffer;
}

const StructuredBuffer& LightsToView::GetLightsBuffer() const
{
    return m_LightsBuffer;
}
