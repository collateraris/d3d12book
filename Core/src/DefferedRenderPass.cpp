#include <DefferedRenderPass.h>

#include <DefferedPass_VS.h>
#include <DefferedPass_PS.h>
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
        t0DiffuseTex,
        t1SpecularTex,
        t2NormalTex,
        NumRootParameters
    };
}

DefferedRenderPass::DefferedRenderPass()
{

}

DefferedRenderPass::~DefferedRenderPass()
{

}

void DefferedRenderPass::LoadContent(RenderPassBaseInfo* info)
{
	DefferedRenderPassInfo* drpInfo = dynamic_cast<DefferedRenderPassInfo*>(info);

    UINT width = drpInfo->m_Width;
    UINT height = drpInfo->m_Height;

    m_ScissorRect = { 0, 0, width, height };

    for (int i = EGBuffer::G_AlbedoSpecular; i < EGBuffer::G_MAX; ++i)
    {
        auto format = drpInfo->rtvFormats.RTFormats[i];
        auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height);
        colorDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        D3D12_CLEAR_VALUE colorClearValue;
        colorClearValue.Format = colorDesc.Format;
        colorClearValue.Color[0] = 0.0f;
        colorClearValue.Color[1] = 0.0f;
        colorClearValue.Color[2] = 0.0f;
        colorClearValue.Color[3] = 1.0f;

        Texture GBufferTexture = core::Texture(colorDesc, &colorClearValue,
            TextureUsage::RenderTarget,
            L"GBuffer");

        m_GBuffer.AttachTexture(static_cast<AttachmentPoint>(i), GBufferTexture);
    }

    // Create a depth buffer for the HDR render target.
    auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(drpInfo->depthBufferFormat, width, height);
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE depthClearValue;
    depthClearValue.Format = depthDesc.Format;
    depthClearValue.DepthStencil = { 1.0f, 0 };

    core::Texture depthTexture = core::Texture(depthDesc, &depthClearValue,
        TextureUsage::Depth,
        L"Depth Render Target");

    m_GBuffer.AttachTexture(core::AttachmentPoint::DepthStencil, depthTexture);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDesc.Texture2D.PlaneSlice = 0;

    m_GBuffer.GetTexture(core::AttachmentPoint::DepthStencil).GetShaderResourceView(&srvDesc);
    m_DepthBufferSRV = srvDesc;


    CD3DX12_ROOT_PARAMETER1 rootParameters[ComputeParams::NumRootParameters];
    rootParameters[ComputeParams::b0MatCB].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
    CD3DX12_DESCRIPTOR_RANGE1 diffuseTexDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    rootParameters[ComputeParams::t0DiffuseTex].InitAsDescriptorTable(1, &diffuseTexDescrRange, D3D12_SHADER_VISIBILITY_PIXEL);
    CD3DX12_DESCRIPTOR_RANGE1 specularTexDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
    rootParameters[ComputeParams::t1SpecularTex].InitAsDescriptorTable(1, &specularTexDescrRange, D3D12_SHADER_VISIBILITY_PIXEL);
    CD3DX12_DESCRIPTOR_RANGE1 normalTexDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);
    rootParameters[ComputeParams::t2NormalTex].InitAsDescriptorTable(1, &normalTexDescrRange, D3D12_SHADER_VISIBILITY_PIXEL);

    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_STATIC_SAMPLER_DESC linearRepeatSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(ComputeParams::NumRootParameters, rootParameters, 1, &linearRepeatSampler, rootSignatureFlags);

    m_RootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, drpInfo->rootSignatureVersion);

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
    pipelineStateStream.VS = {g_DefferedPass_VS, sizeof(g_DefferedPass_VS)};
    pipelineStateStream.PS = {g_DefferedPass_PS, sizeof(g_DefferedPass_PS)};
    pipelineStateStream.DSVFormat = drpInfo->depthBufferFormat;
    pipelineStateStream.RTVFormats = drpInfo->rtvFormats;
    pipelineStateStream.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };

    auto& device = GetApp().GetDevice();
    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState)));
}

void DefferedRenderPass::OnUpdate(std::shared_ptr<CommandList>& commandList, UpdateEventArgs& e)
{

}

void DefferedRenderPass::OnPreRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e)
{
    // Clear the render targets.
    {
        FLOAT clearColor[] = { 0.f, 0.f, 0.f, 1.0f };
        for (int i = EGBuffer::G_AlbedoSpecular; i < EGBuffer::G_MAX; ++i)
            commandList->ClearTexture(m_GBuffer.GetTexture(static_cast<AttachmentPoint>(i)), clearColor);
        commandList->ClearDepthStencilTexture(m_GBuffer.GetTexture(core::AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL);
    }

    commandList->SetGraphicsRootSignature(m_RootSignature);
    commandList->SetPipelineState(m_PipelineState);

    commandList->SetRenderTarget(m_GBuffer);
    commandList->SetViewport(m_GBuffer.GetViewport());
    commandList->SetScissorRect(m_ScissorRect);
}

void DefferedRenderPass::OnRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e)
{

}

void DefferedRenderPass::AttachDiffuseTex(std::shared_ptr<CommandList>& commandList, const Texture& tex)
{
    commandList->SetShaderResourceView(ComputeParams::t0DiffuseTex, 0, tex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void DefferedRenderPass::AttachSpecularTex(std::shared_ptr<CommandList>& commandList, const Texture& tex)
{
    commandList->SetShaderResourceView(ComputeParams::t1SpecularTex, 0, tex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void DefferedRenderPass::AttachNormalTex(std::shared_ptr<CommandList>& commandList, const Texture& tex)
{
    commandList->SetShaderResourceView(ComputeParams::t2NormalTex, 0, tex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

const Texture& DefferedRenderPass::GetDepthBuffer() const
{
    return m_GBuffer.GetTexture(core::AttachmentPoint::DepthStencil);
}

const Texture& DefferedRenderPass::GetGBuffer(EGBuffer tex) const
{
    assert(tex != EGBuffer::G_MAX);
    return m_GBuffer.GetTexture(static_cast<AttachmentPoint>(tex));
}

const D3D12_SHADER_RESOURCE_VIEW_DESC& DefferedRenderPass::GetDepthBufferSRV() const
{
    return m_DepthBufferSRV;
}
