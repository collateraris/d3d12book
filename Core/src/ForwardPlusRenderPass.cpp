#include <ForwardPlusRenderPass.h>

#include <ForwardPlus_VS.h>
#include <ForwardPlus_PS.h>
#include <Mesh.h>
#include <DX12LibPCH.h>
#include <Application.h>
#include <Texture.h>
#include <StructuredBuffer.h>

using namespace dx12demo::core;

namespace ComputeParams
{
    enum
    {
        b0MatCB,
        t0AmbientTex,
        t1DiffuseTex,
        t2SpecularTex,
        t2NormalTex,
        t4LightGridTex,
        t5LightsSB,
        t6LightIndexListSB,
        NumRootParameters
    };
}

ForwardPlusRenderPass::ForwardPlusRenderPass()
{

}

ForwardPlusRenderPass::~ForwardPlusRenderPass()
{

}

void ForwardPlusRenderPass::LoadContent(RenderPassBaseInfo* info)
{
	ForwardPlusRenderPassInfo* fpInfo = dynamic_cast<ForwardPlusRenderPassInfo*>(info);

    CD3DX12_ROOT_PARAMETER1 rootParameters[ComputeParams::NumRootParameters];
    rootParameters[ComputeParams::b0MatCB].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
    CD3DX12_DESCRIPTOR_RANGE1 ambientTexDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    rootParameters[ComputeParams::t0AmbientTex].InitAsDescriptorTable(1, &ambientTexDescrRange, D3D12_SHADER_VISIBILITY_PIXEL);
    CD3DX12_DESCRIPTOR_RANGE1 diffuseTexDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
    rootParameters[ComputeParams::t1DiffuseTex].InitAsDescriptorTable(1, &diffuseTexDescrRange, D3D12_SHADER_VISIBILITY_PIXEL);
    CD3DX12_DESCRIPTOR_RANGE1 specularTexDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);
    rootParameters[ComputeParams::t2SpecularTex].InitAsDescriptorTable(1, &specularTexDescrRange, D3D12_SHADER_VISIBILITY_PIXEL);
    CD3DX12_DESCRIPTOR_RANGE1 normalTexDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3);
    rootParameters[ComputeParams::t2NormalTex].InitAsDescriptorTable(1, &normalTexDescrRange, D3D12_SHADER_VISIBILITY_PIXEL);
    CD3DX12_DESCRIPTOR_RANGE1 lightGridTexDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4);
    rootParameters[ComputeParams::t4LightGridTex].InitAsDescriptorTable(1, &lightGridTexDescrRange, D3D12_SHADER_VISIBILITY_PIXEL);
    CD3DX12_DESCRIPTOR_RANGE1 lightsSBDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5);
    rootParameters[ComputeParams::t5LightsSB].InitAsDescriptorTable(1, &lightsSBDescrRange, D3D12_SHADER_VISIBILITY_PIXEL);
    CD3DX12_DESCRIPTOR_RANGE1 lightIndexListSBDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 6);
    rootParameters[ComputeParams::t6LightIndexListSB].InitAsDescriptorTable(1, &lightIndexListSBDescrRange, D3D12_SHADER_VISIBILITY_PIXEL);

    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_STATIC_SAMPLER_DESC linearRepeatSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(ComputeParams::NumRootParameters, rootParameters, 1, &linearRepeatSampler, rootSignatureFlags);

    m_RootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, fpInfo->rootSignatureVersion);

    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_VS VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS PS;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DepthStencilState;
    } pipelineStateStream;

    pipelineStateStream.pRootSignature = m_RootSignature.GetRootSignature().Get();
    pipelineStateStream.InputLayout = { PosNormTexExtendedVertex::InputElementsExtended, PosNormTexExtendedVertex::InputElementCountExtended };
    pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateStream.VS = {g_ForwardPlus_VS, sizeof(g_ForwardPlus_VS)};
    pipelineStateStream.PS = {g_ForwardPlus_PS, sizeof(g_ForwardPlus_PS)};
    pipelineStateStream.DSVFormat = fpInfo->depthBufferFormat;
    pipelineStateStream.RTVFormats = fpInfo->rtvFormats;
    pipelineStateStream.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };

    auto& device = GetApp().GetDevice();
    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState)));
}

void ForwardPlusRenderPass::OnUpdate(std::shared_ptr<CommandList>& commandList, UpdateEventArgs& e)
{

}

void ForwardPlusRenderPass::OnPreRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e)
{
    commandList->SetGraphicsRootSignature(m_RootSignature);
    commandList->SetPipelineState(m_PipelineState);
}

void ForwardPlusRenderPass::OnRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e)
{

}

void ForwardPlusRenderPass::AttachAmbientTex(std::shared_ptr<CommandList>& commandList, const Texture& tex)
{
    commandList->SetShaderResourceView(ComputeParams::t0AmbientTex, 0, tex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void ForwardPlusRenderPass::AttachDiffuseTex(std::shared_ptr<CommandList>& commandList, const Texture& tex)
{
    commandList->SetShaderResourceView(ComputeParams::t1DiffuseTex, 0, tex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void ForwardPlusRenderPass::AttachSpecularTex(std::shared_ptr<CommandList>& commandList, const Texture& tex)
{
    commandList->SetShaderResourceView(ComputeParams::t2SpecularTex, 0, tex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void ForwardPlusRenderPass::AttachNormalTex(std::shared_ptr<CommandList>& commandList, const Texture& tex)
{
    commandList->SetShaderResourceView(ComputeParams::t2NormalTex, 0, tex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void ForwardPlusRenderPass::AttachLightGridTex(std::shared_ptr<CommandList>& commandList, const Texture& tex)
{
    commandList->SetShaderResourceView(ComputeParams::t4LightGridTex, 0, tex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void ForwardPlusRenderPass::AttachLightsSB(std::shared_ptr<CommandList>& commandList, const StructuredBuffer& sb)
{
    commandList->SetShaderResourceView(ComputeParams::t5LightsSB, 0, sb, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void ForwardPlusRenderPass::AttachLightIndexListSB(std::shared_ptr<CommandList>& commandList, const StructuredBuffer& sb)
{
    commandList->SetShaderResourceView(ComputeParams::t6LightIndexListSB, 0, sb, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}