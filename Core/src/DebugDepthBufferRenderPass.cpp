#include <DebugDepthBufferRenderPass.h>

#include <DX12LibPCH.h>
#include <Application.h>
#include <CommandQueue.h>
#include <CommandList.h>
#include <DebugDepthBuffer_PS.h>
#include <DebugDepthBuffer_VS.h>
#include <Mesh.h>

using namespace dx12demo::core;

namespace ComputeParams
{
	enum
	{
		t0DebugTex,
		NumRootParameters
	};
}

DebugDepthBufferRenderPass::DebugDepthBufferRenderPass()
{

}

DebugDepthBufferRenderPass::~DebugDepthBufferRenderPass()
{

}

void DebugDepthBufferRenderPass::LoadContent(RenderPassBaseInfo* info)
{
	DebugDepthBufferRenderPassInfo* depthRPInfo = dynamic_cast<DebugDepthBufferRenderPassInfo*>(info);

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	CD3DX12_ROOT_PARAMETER1 rootParameters[ComputeParams::NumRootParameters];
	CD3DX12_DESCRIPTOR_RANGE1 depthTexDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	rootParameters[ComputeParams::t0DebugTex].InitAsDescriptorTable(1, &depthTexDescrRange, D3D12_SHADER_VISIBILITY_PIXEL);

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
	rootSignatureDescription.Init_1_1(ComputeParams::NumRootParameters, rootParameters, 1, &linearWrap, rootSignatureFlags);

	m_RootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, depthRPInfo->rootSignatureVersion);

	struct PipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
		CD3DX12_PIPELINE_STATE_STREAM_VS VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS PS;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
	} pipelineStateStream;

	pipelineStateStream.pRootSignature = m_RootSignature.GetRootSignature().Get();
	pipelineStateStream.InputLayout = { core::PosNormTexVertex::InputElements, core::PosNormTexVertex::InputElementCount };
	pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateStream.VS = { g_DebugDepthBuffer_VS, sizeof(g_DebugDepthBuffer_VS) };
	pipelineStateStream.PS = { g_DebugDepthBuffer_PS, sizeof(g_DebugDepthBuffer_PS) };
	pipelineStateStream.RTVFormats = depthRPInfo->rtvFormats;

	D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
		sizeof(PipelineStateStream), &pipelineStateStream
	};

	auto& device = GetApp().GetDevice();
	ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState)));

	m_QuadMesh = Mesh::CreateQuad(*depthRPInfo->commandList, -1.0f, 1.0f, 2.0f, 2.0f, 0.0f, true);
}

void DebugDepthBufferRenderPass::OnUpdate(std::shared_ptr<CommandList>& commandList, UpdateEventArgs& e)
{

}

void DebugDepthBufferRenderPass::OnPreRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e)
{
	commandList->SetPipelineState(m_PipelineState);
	commandList->SetGraphicsRootSignature(m_RootSignature);
}

void DebugDepthBufferRenderPass::OnRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e)
{
	m_QuadMesh->Render(commandList);
}

