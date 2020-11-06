#include <GridViewFrustums.h>

#include <DX12LibPCH.h>
#include <Application.h>
#include <CommandQueue.h>
#include <GridViewFrustums_CS.h>

using namespace dx12demo::core;

namespace ComputeParams
{
    enum
    {
        OutFrustumUAV,
        ScreenToViewParamsCB,
        DispatchParamsCB,
        NumRootParameters
    };
}

GridViewFrustum::GridViewFrustum()
{
    auto device = GetApp().GetDevice();

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    CD3DX12_ROOT_PARAMETER1 rootParameters[ComputeParams::NumRootParameters];
    CD3DX12_DESCRIPTOR_RANGE1 outFrustumDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
    rootParameters[ComputeParams::OutFrustumUAV].InitAsDescriptorTable(1, &outFrustumDescrRange);
    rootParameters[ComputeParams::ScreenToViewParamsCB].InitAsConstantBufferView(0, 0);
    rootParameters[ComputeParams::DispatchParamsCB].InitAsConstantBufferView(1, 0);

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
    pipelineStateStream.CS = { g_GridViewFrustums_CS, sizeof(g_GridViewFrustums_CS) };

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
       sizeof(PipelineStateStream), &pipelineStateStream
    };

    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState)));
}

GridViewFrustum::~GridViewFrustum()
{

}

void GridViewFrustum::Compute(const ScreenToViewParams& params, const DispatchParams& dispatchPar)
{
    auto commandQueue = GetApp().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE);
    auto commandList = commandQueue->GetCommandList();
    commandList->SetComputeRootSignature(m_RootSignature);
    commandList->SetPipelineState(m_PipelineState);

    const auto& numThreads = dispatchPar.m_NumThreads;
    auto frustumsNum = numThreads.x * numThreads.y * numThreads.z;
    m_GridFrustums.resize(frustumsNum);
    commandList->CopyStructuredBuffer(m_GridFrustumBuffer, m_GridFrustums);

    commandList->SetUnorderedAccessView(ComputeParams::OutFrustumUAV, 0, m_GridFrustumBuffer);
    commandList->SetComputeDynamicConstantBuffer(ComputeParams::ScreenToViewParamsCB, params);
    commandList->SetComputeDynamicConstantBuffer(ComputeParams::DispatchParamsCB, dispatchPar);

    commandList->Dispatch(numThreads.x, numThreads.y, numThreads.z);

    /*

    auto& device = GetApp().GetDevice();

    ComPtr<ID3D12Resource> mReadBackBuffer;
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(sizeof(FrustumPrimitive) * frustumsNum),
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&mReadBackBuffer)));


    commandList->TransitionBarrier(m_GridFrustumBuffer.GetD3D12Resource(), D3D12_RESOURCE_STATE_COPY_SOURCE);
    commandList->CopyResource(mReadBackBuffer, m_GridFrustumBuffer.GetD3D12Resource());

    */

    auto fenceValue = commandQueue->ExecuteCommandList(commandList);
    commandQueue->WaitForFenceValue(fenceValue);


    /*
    FrustumPrimitive* mappedData = nullptr;
    ThrowIfFailed(mReadBackBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedData)));

    mReadBackBuffer->Unmap(0, nullptr);
    */
}

const StructuredBuffer& GridViewFrustum::GetGridFrustums() const
{
	return m_GridFrustumBuffer;
}