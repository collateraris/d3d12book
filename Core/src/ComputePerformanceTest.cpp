#include <ComputePerformanceTest.h>

#include <DX12LibPCH.h>
#include <Application.h>
#include <CommandQueue.h>
#include <Sum_CS.h>

using namespace dx12demo::core;

namespace ComputeParams
{
    enum
    {
        u0InputArrSB,
        u1OutputParSB,
        NumRootParameters
    };
}

ComputePerformanceTest::ComputePerformanceTest()
{
    auto device = GetApp().GetDevice();

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    CD3DX12_ROOT_PARAMETER1 rootParameters[ComputeParams::NumRootParameters];

    CD3DX12_DESCRIPTOR_RANGE1 inputArrDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
    rootParameters[ComputeParams::u0InputArrSB].InitAsDescriptorTable(1, &inputArrDescrRange);
    CD3DX12_DESCRIPTOR_RANGE1 outputParDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);
    rootParameters[ComputeParams::u1OutputParSB].InitAsDescriptorTable(1, &outputParDescrRange);

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
    pipelineStateStream.CS = { g_main, sizeof(g_main) };

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
       sizeof(PipelineStateStream), &pipelineStateStream
    };

    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState)));

    auto commandQueue = GetApp().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    auto commandList = commandQueue->GetCommandList();

    auto fenceValue = commandQueue->ExecuteCommandList(commandList);
    commandQueue->WaitForFenceValue(fenceValue);
}

ComputePerformanceTest::~ComputePerformanceTest()
{

}

void ComputePerformanceTest::StartCompute(std::shared_ptr<CommandList>& commandList)
{
    commandList->SetComputeRootSignature(m_RootSignature);
    commandList->SetPipelineState(m_PipelineState);
}


void ComputePerformanceTest::InitBuffers(std::shared_ptr<CommandList>& commandList)
{  
    std::vector<uint32_t> bufferArr(262144, 0);
    for (auto& buffer : bufferArr)
    {
        buffer = 1;
    }

    commandList->CopyStructuredBuffer(m_InputArrBuffer, bufferArr);

    std::vector<uint32_t> bufferArr2(1, 0);

    commandList->CopyStructuredBuffer(m_OutputParBuffer, bufferArr2);


}
void ComputePerformanceTest::Compute(std::shared_ptr<CommandList>& commandList)
{
    commandList->SetUnorderedAccessView(ComputeParams::u0InputArrSB, 0, m_InputArrBuffer);
    commandList->SetUnorderedAccessView(ComputeParams::u1OutputParSB, 0, m_OutputParBuffer);

    //commandList->Dispatch(1, 1, 1);
    commandList->Dispatch(256, 1, 1);
 
}
