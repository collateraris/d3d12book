#include <PrecomputeParticleDensityForAtmScat.h>

#include <DX12LibPCH.h>
#include <Application.h>
#include <Mesh.h>

#include <Quad_VS.h>
#include <PrecomputeParticleDensityForAtmScat_PS.h>

using namespace dx12demo::core;

namespace ComputeParams
{
    enum
    {
        t0AtmPar,
        NumRootParameters
    };
}

PrecomputeParticleDensityForAtmScatRP::PrecomputeParticleDensityForAtmScatRP()
{
}

PrecomputeParticleDensityForAtmScatRP::~PrecomputeParticleDensityForAtmScatRP()
{
}

void PrecomputeParticleDensityForAtmScatRP::LoadContent(RenderPassBaseInfo* info)
{
	PrecomputeParticleDensityForAtmScatRenderPassInfo* quadInfo = dynamic_cast<PrecomputeParticleDensityForAtmScatRenderPassInfo*>(info);

    int bufferW = quadInfo->width;
    int bufferH = quadInfo->height;

    m_ScissorRect = { 0, 0, bufferW, bufferH };
    m_Viewport = { 0.0f, 0.0f, static_cast<float>(bufferW), static_cast<float>(bufferH), 0.0f, 1.0f };

    DXGI_FORMAT RTVFormat = DXGI_FORMAT_R32G32_FLOAT;

    auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(RTVFormat, bufferW, bufferH);
    colorDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE colorClearValue;
    colorClearValue.Format = colorDesc.Format;
    colorClearValue.Color[0] = 0.4f;
    colorClearValue.Color[1] = 0.6f;
    colorClearValue.Color[2] = 0.9f;
    colorClearValue.Color[3] = 1.0f;

    core::Texture ScreenTexture = core::Texture(colorDesc, &colorClearValue,
        TextureUsage::RenderTarget,
        L"PrecomputeParticleDensityForAtmScat Texture");

    m_RenderTarget.AttachTexture(core::AttachmentPoint::Color0, ScreenTexture);

    CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    CD3DX12_ROOT_PARAMETER1 rootParameters[ComputeParams::NumRootParameters];
    rootParameters[ComputeParams::t0AtmPar].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(1, rootParameters, 0, nullptr);

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
    pipelineStateStream.PS = { g_PrecomputeParticleDensityForAtmScat_PS, sizeof(g_PrecomputeParticleDensityForAtmScat_PS) };
    pipelineStateStream.Rasterizer = rasterizerDesc;
    pipelineStateStream.RTVFormats = m_RenderTarget.GetRenderTargetFormats();

    D3D12_PIPELINE_STATE_STREAM_DESC PipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };

    auto& device = GetApp().GetDevice();
    ThrowIfFailed(device->CreatePipelineState(&PipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState)));
}

void PrecomputeParticleDensityForAtmScatRP::OnUpdate(std::shared_ptr<CommandList>& commandList, UpdateEventArgs& e)
{

}

void PrecomputeParticleDensityForAtmScatRP::OnPreRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e)
{
    commandList->SetViewport(m_Viewport);
    commandList->SetScissorRect(m_ScissorRect);

    commandList->SetRenderTarget(m_RenderTarget);

    commandList->SetPipelineState(m_PipelineState);
    commandList->SetGraphicsRootSignature(m_RootSignature);
    commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void PrecomputeParticleDensityForAtmScatRP::OnRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e)
{
    commandList->Draw(3);
}

const Texture& PrecomputeParticleDensityForAtmScatRP::GetPrecomputeParticleDensity() const
{
    return m_RenderTarget.GetTexture(core::AttachmentPoint::Color0);
}

void PrecomputeParticleDensityForAtmScatRP::BindAtmosphereParams(std::shared_ptr<CommandList>& commandList, StructuredBuffer& atmPar)
{
    commandList->SetShaderResourceView(ComputeParams::t0AtmPar, 0, atmPar);
}