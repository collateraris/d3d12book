#include <QuadRenderPass.h>

#include <DX12LibPCH.h>
#include <Application.h>
#include <Mesh.h>
#include <Quad_PS.h>
#include <Quad_VS.h>

using namespace dx12demo::core;

namespace ComputeParams
{
    enum
    {
        t0Tex,
        NumRootParameters
    };
}

QuadRenderPass::QuadRenderPass()
{
}

QuadRenderPass::~QuadRenderPass()
{
}

void QuadRenderPass::LoadContent(RenderPassBaseInfo* info)
{
	QuadRenderPassInfo* quadInfo = dynamic_cast<QuadRenderPassInfo*>(info);

    CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    CD3DX12_ROOT_PARAMETER1 rootParameters[ComputeParams::NumRootParameters];
    rootParameters[ComputeParams::t0Tex].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_STATIC_SAMPLER_DESC linearClampsSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(1, rootParameters, 1, &linearClampsSampler);

    m_RootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, quadInfo->rootSignatureVersion);

    CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_VS VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS PS;
        CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER Rasterizer;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
    } pipelineStateStream;

    pipelineStateStream.pRootSignature = m_RootSignature.GetRootSignature().Get();
    pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateStream.VS = { g_Quad_VS, sizeof(g_Quad_VS) };
    pipelineStateStream.PS = { g_Quad_PS, sizeof(g_Quad_PS) };
    pipelineStateStream.Rasterizer = rasterizerDesc;
    pipelineStateStream.RTVFormats = quadInfo->rtvFormats;

    D3D12_PIPELINE_STATE_STREAM_DESC PipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };

    auto& device = GetApp().GetDevice();
    ThrowIfFailed(device->CreatePipelineState(&PipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState)));
}

void QuadRenderPass::OnUpdate(std::shared_ptr<CommandList>& commandList, UpdateEventArgs& e)
{

}

void QuadRenderPass::OnPreRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e)
{
    commandList->SetPipelineState(m_PipelineState);
    commandList->SetGraphicsRootSignature(m_RootSignature);
    commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void QuadRenderPass::OnRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e)
{
    commandList->Draw(3);
}