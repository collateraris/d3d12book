#include <SkyDoom.h>

#include <Application.h>
#include <CommandQueue.h>
#include <CommandList.h>
#include <DX12LibPCH.h>

#include <cassert>

#include <Skybox_VS.h>
#include <SkyDoomGradient_PS.h>

using namespace dx12demo::core;

std::shared_ptr<SkyDoomCommon> SkyDoomFabric::GetGradientType(std::shared_ptr<Camera>& camera, D3D12_RT_FORMAT_ARRAY rtvFormats,
	DXGI_FORMAT depthBufferFormat, D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion)
{
	std::shared_ptr<SkyDoomCommon> skydoom_common = std::make_shared<SkyDoomCommon>();

	SkyDoomCommonRenderPassInfo info;

	auto& app = GetApplication();
	auto& device = app.GetDevice();
	auto commandQueue = app.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto commandList = commandQueue->GetCommandList();

	info.camera = camera;
	info.skydoomMesh = Mesh::CreateSphere(*commandList, 1, 6, true);

	info.rootSignature = std::make_unique<RootSignature>();

	D3D12_INPUT_ELEMENT_DESC inputLayout[1] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	// Allow input layout and deny unnecessary access to certain pipeline stages.
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	CD3DX12_ROOT_PARAMETER1 rootParameters[1];
	rootParameters[0].InitAsConstants(sizeof(DirectX::XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
	rootSignatureDescription.Init_1_1(1, rootParameters, 0, nullptr, rootSignatureFlags);

	info.rootSignature->SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, rootSignatureVersion);

	// Setup the Skybox pipeline state.
	struct SkyDoomPipelineState
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
		CD3DX12_PIPELINE_STATE_STREAM_VS VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS PS;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
	} skydoomPipelineStateStream;

	skydoomPipelineStateStream.pRootSignature = info.rootSignature->GetRootSignature().Get();
	skydoomPipelineStateStream.InputLayout = { inputLayout, 1 };
	skydoomPipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	skydoomPipelineStateStream.VS = { g_Skybox_VS, sizeof(g_Skybox_VS) };
	skydoomPipelineStateStream.PS = { g_SkyDoomGradient_PS, sizeof(g_SkyDoomGradient_PS) };
	skydoomPipelineStateStream.RTVFormats = rtvFormats;

	D3D12_PIPELINE_STATE_STREAM_DESC skyboxPipelineStateStreamDesc = {
		sizeof(SkyDoomPipelineState), &skydoomPipelineStateStream
	};

	ThrowIfFailed(device->CreatePipelineState(&skyboxPipelineStateStreamDesc, IID_PPV_ARGS(&info.pipelineState)));

	auto fenceValue = commandQueue->ExecuteCommandList(commandList);
	commandQueue->WaitForFenceValue(fenceValue);

	skydoom_common->LoadContent(&info);

	return skydoom_common;
}

SkyDoomCommon::SkyDoomCommon()
{

}

SkyDoomCommon::~SkyDoomCommon()
{

}

void SkyDoomCommon::LoadContent(RenderPassBaseInfo* info)
{
	SkyDoomCommonRenderPassInfo* sdInfo = dynamic_cast<SkyDoomCommonRenderPassInfo*>(info);

	m_RootSignature = sdInfo->rootSignature;
	m_PipelineState = sdInfo->pipelineState;
	m_SkyDoomMesh = sdInfo->skydoomMesh;
	m_Camera = sdInfo->camera;

}

void SkyDoomCommon::OnUpdate(std::shared_ptr<CommandList>& commandList, UpdateEventArgs& e)
{
	auto camPos = m_Camera->GetPosition();
	DirectX::XMMATRIX world = DirectX::XMMatrixTranslationFromVector(camPos);
	DirectX::XMMATRIX view = m_Camera->get_ViewMatrix();
	DirectX::XMMATRIX proj = m_Camera->get_ProjectionMatrix();

	m_MVP = world * view * proj;
}

void SkyDoomCommon::OnPreRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e)
{
}

void SkyDoomCommon::OnRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e)
{
	commandList->SetPipelineState(m_PipelineState);
	commandList->SetGraphicsRootSignature(*m_RootSignature);

	commandList->SetGraphics32BitConstants(0, m_MVP);

	m_SkyDoomMesh->Render(commandList);
}
