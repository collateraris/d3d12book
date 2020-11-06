#include <DepthBufferRenderPass.h>

#include <DX12LibPCH.h>
#include <Application.h>
#include <CommandList.h>
#include <Mesh.h>
#include <DepthBuffer_VS.h>
#include <DepthBuffer_PS.h>

using namespace dx12demo::core;

namespace ComputeParams
{
	enum
	{
		b0MatCB,
		NumRootParameters
	};
}

DepthBufferRenderPass::DepthBufferRenderPass()
{

}

DepthBufferRenderPass::~DepthBufferRenderPass()
{

}

void DepthBufferRenderPass::LoadContent(RenderPassBaseInfo* info)
{
	DepthBufferRenderPassInfo* depthRPInfo = dynamic_cast<DepthBufferRenderPassInfo*>(info);

	int bufferW = depthRPInfo->bufferW;
	int bufferH = depthRPInfo->bufferH;

	m_ScissorRect = { 0, 0, bufferW, bufferH };
	m_Viewport = { 0.0f, 0.0f, static_cast<float>(bufferW), static_cast<float>(bufferH), 0.0f, 1.0f };

	D3D12_RESOURCE_DESC resDesc = {};
	D3D12_CLEAR_VALUE clearValue = {};
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};

	if (depthRPInfo->bUseCustomDepthBufferDesc)
	{
		resDesc = depthRPInfo->resDesc;
		clearValue = depthRPInfo->clearValue;
		srvDesc = depthRPInfo->srvDesc;
		dsvDesc = depthRPInfo->dsvDesc;
	}
	else
	{

		resDesc = {};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resDesc.Alignment = 0;
		resDesc.Width = bufferW;
		resDesc.Height = bufferH;
		resDesc.DepthOrArraySize = 1;
		resDesc.MipLevels = 1;
		resDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		resDesc.SampleDesc.Count = 1;
		resDesc.SampleDesc.Quality = 0;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		clearValue = {};
		clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		clearValue.DepthStencil.Depth = 1.0f;
		clearValue.DepthStencil.Stencil = 0;

		srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		srvDesc.Texture2D.PlaneSlice = 0;

		// Create DSV to resource so we can render to the shadow map.
		dsvDesc = {};
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsvDesc.Texture2D.MipSlice = 0;
	}

	Texture depthTexture = core::Texture(resDesc, &clearValue,
		TextureUsage::Depth,
		L"Depth Render Target",
		nullptr, &dsvDesc);

	m_RenderTarget.AttachTexture(AttachmentPoint::DepthStencil, depthTexture);
	auto& depthBuffer = GetDepthBuffer();
	depthBuffer.GetShaderResourceView(&srvDesc);

	m_SRV = srvDesc;

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	CD3DX12_ROOT_PARAMETER1 rootParameters[ComputeParams::NumRootParameters];
	rootParameters[ComputeParams::b0MatCB].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
	rootSignatureDescription.Init_1_1(ComputeParams::NumRootParameters, rootParameters, 0, nullptr, rootSignatureFlags);

	m_RootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, depthRPInfo->rootSignatureVersion);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateStream;
	ZeroMemory(&pipelineStateStream, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	pipelineStateStream.pRootSignature = m_RootSignature.GetRootSignature().Get();
	pipelineStateStream.InputLayout = { core::PosNormTexExtendedVertex::InputElementsExtended, core::PosNormTexExtendedVertex::InputElementCountExtended };
	pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateStream.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	pipelineStateStream.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	pipelineStateStream.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	pipelineStateStream.SampleMask = UINT_MAX;
	pipelineStateStream.VS = { g_DepthBuffer_VS, sizeof(g_DepthBuffer_VS) };
	pipelineStateStream.PS = { g_DepthBuffer_PS, sizeof(g_DepthBuffer_PS) };
	pipelineStateStream.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	pipelineStateStream.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	pipelineStateStream.NumRenderTargets = 0;
	pipelineStateStream.SampleDesc.Count = 1;
	pipelineStateStream.SampleDesc.Quality = 0;

	auto& device = GetApp().GetDevice();
	ThrowIfFailed(device->CreateGraphicsPipelineState(&pipelineStateStream, IID_PPV_ARGS(&m_PipelineState)));
}

void DepthBufferRenderPass::OnUpdate(std::shared_ptr<CommandList>& commandList, UpdateEventArgs& e)
{

}

void DepthBufferRenderPass::OnRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e)
{
	commandList->SetViewport(m_Viewport);
	commandList->SetScissorRect(m_ScissorRect);

	commandList->SetRenderTargetWriteDepthBufferOnly(m_RenderTarget);
	commandList->ClearDepthStencilTexture(GetDepthBuffer(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL);

	commandList->SetPipelineState(m_PipelineState);
	commandList->SetGraphicsRootSignature(m_RootSignature);
}

const Texture& DepthBufferRenderPass::GetDepthBuffer() const
{
	return m_RenderTarget.GetTexture(core::AttachmentPoint::DepthStencil);
}

const D3D12_SHADER_RESOURCE_VIEW_DESC& DepthBufferRenderPass::GetSRV() const
{
	return m_SRV;
}