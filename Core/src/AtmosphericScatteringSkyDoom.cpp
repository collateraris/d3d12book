#include <AtmosphericScatteringSkyDoom.h>

#include <Application.h>
#include <CommandQueue.h>
#include <CommandList.h>
#include <DX12LibPCH.h>

#include <cassert>

#include <AtmosphericScatteringSkyDoom_VS.h>
#include <AtmosphericScatteringSkyDoom_PS.h>

using namespace dx12demo::core;

namespace ShaderParams
{
	enum
	{
		b0MatCB,
		t0AtmData,
		NumRootParameters,
	};
}

AtmosphericScatteringSkyDoom::AtmosphericScatteringSkyDoom()
{

}

AtmosphericScatteringSkyDoom::~AtmosphericScatteringSkyDoom()
{

}

void AtmosphericScatteringSkyDoom::LoadContent(RenderPassBaseInfo* info)
{
	AtmosphericScatteringSkyDoomRenderPassInfo* sdInfo = dynamic_cast<AtmosphericScatteringSkyDoomRenderPassInfo*>(info);

	m_Camera = sdInfo->camera;

	auto& app = GetApp();
	auto& device = app.GetDevice();
	auto commandQueue = app.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto commandList = commandQueue->GetCommandList();

	{
		m_SkyDoomMesh = Mesh::CreateSphere(*commandList, 1, 6, true);

		auto fenceValue = commandQueue->ExecuteCommandList(commandList);
		commandQueue->WaitForFenceValue(fenceValue);
	}

	{
		D3D12_INPUT_ELEMENT_DESC inputLayout[1] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		// Allow input layout and deny unnecessary access to certain pipeline stages.
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		CD3DX12_DESCRIPTOR_RANGE1 atmDataDescRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
		CD3DX12_ROOT_PARAMETER1 rootParameters[ShaderParams::NumRootParameters];
		rootParameters[ShaderParams::b0MatCB].InitAsConstants(sizeof(DirectX::XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
		rootParameters[ShaderParams::t0AtmData].InitAsDescriptorTable(1, &atmDataDescRange, D3D12_SHADER_VISIBILITY_PIXEL);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
		rootSignatureDescription.Init_1_1(ShaderParams::NumRootParameters, rootParameters, 0, nullptr, rootSignatureFlags);

		m_RootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, sdInfo->rootSignatureVersion);

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

		skydoomPipelineStateStream.pRootSignature = m_RootSignature.GetRootSignature().Get();
		skydoomPipelineStateStream.InputLayout = { inputLayout, 1 };
		skydoomPipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		skydoomPipelineStateStream.VS = { g_AtmosphericScatteringSkyDoom_VS, sizeof(g_AtmosphericScatteringSkyDoom_VS) };
		skydoomPipelineStateStream.PS = { g_AtmosphericScatteringSkyDoom_PS, sizeof(g_AtmosphericScatteringSkyDoom_PS) };
		skydoomPipelineStateStream.RTVFormats = sdInfo->rtvFormats;

		D3D12_PIPELINE_STATE_STREAM_DESC skyboxPipelineStateStreamDesc = {
			sizeof(SkyDoomPipelineState), &skydoomPipelineStateStream
		};

		ThrowIfFailed(device->CreatePipelineState(&skyboxPipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState)));
	}
}

void AtmosphericScatteringSkyDoom::OnUpdate(std::shared_ptr<CommandList>& commandList, UpdateEventArgs& e)
{
	const auto& camPos = m_Camera->GetPosition();
	DirectX::XMMATRIX world = DirectX::XMMatrixTranslationFromVector(camPos);
	DirectX::XMMATRIX view = m_Camera->get_ViewMatrix();
	DirectX::XMMATRIX proj = m_Camera->get_ProjectionMatrix();

	m_MVP = world * view * proj;

	const auto& camDir = m_Camera->GetDirection();
	XMStoreFloat3(&m_AtmData.viewPos, camDir);
}

void AtmosphericScatteringSkyDoom::OnPreRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e)
{
}

void AtmosphericScatteringSkyDoom::OnRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e)
{
	commandList->CopyStructuredBuffer(m_AtmDataBuffer, m_AtmData);

	commandList->SetPipelineState(m_PipelineState);
	commandList->SetGraphicsRootSignature(m_RootSignature);

	commandList->SetGraphics32BitConstants(ShaderParams::b0MatCB, m_MVP);
	commandList->SetShaderResourceView(ShaderParams::t0AtmData, 0, m_AtmDataBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	m_SkyDoomMesh->Render(commandList);
}
