#include <BitmapClouds.h>

#include <Application.h>
#include <CommandQueue.h>
#include <CommandList.h>
#include <DX12LibPCH.h>

#include <cassert>

#include <BitmapClouds_VS.h>
#include <BitmapClouds_PS.h>

using namespace dx12demo::core;

namespace ShaderParams
{
	enum
	{
		b0MatCB,
		t0Cloud1Tex,
		t1Cloud2Tex,
		t2SkySB,
		NumRootParameters,
	};
}


BitmapClouds::BitmapClouds()
{

}

BitmapClouds::~BitmapClouds()
{

}

void BitmapClouds::LoadContent(RenderPassBaseInfo* info)
{
	BitmapCloudsRenderPassInfo* sdInfo = dynamic_cast<BitmapCloudsRenderPassInfo*>(info);

	auto& app = GetApp();
	auto& device = app.GetDevice();
	auto commandQueue = app.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto commandList = commandQueue->GetCommandList();

	m_SkyPlaneMesh = Mesh::CreateSkyPlane(*commandList, sdInfo->skyPlaneResolution, sdInfo->skyPlaneWidth, sdInfo->skyPlaneTop, sdInfo->skyPlaneBottom, sdInfo->textureRepeat, true);

	commandList->LoadTextureFromFile(m_CloudTex1, sdInfo->cloudtex1_path);
	commandList->LoadTextureFromFile(m_CloudTex2, sdInfo->cloudtex2_path);

	commandList->CopyStructuredBuffer(m_SkyBuffer, m_SkyBufferStruct);

	auto fenceValue = commandQueue->ExecuteCommandList(commandList);
	commandQueue->WaitForFenceValue(fenceValue);

	m_Camera = sdInfo->camera;

	{
		D3D12_INPUT_ELEMENT_DESC inputLayout[2] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// Allow input layout and deny unnecessary access to certain pipeline stages.
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		CD3DX12_DESCRIPTOR_RANGE1 cloudTex1DescRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
		CD3DX12_DESCRIPTOR_RANGE1 cloudTex2DescRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
		CD3DX12_DESCRIPTOR_RANGE1 skySBDescRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);
		CD3DX12_ROOT_PARAMETER1 rootParameters[ShaderParams::NumRootParameters];

		rootParameters[ShaderParams::b0MatCB].InitAsConstants(sizeof(DirectX::XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
		rootParameters[ShaderParams::t0Cloud1Tex].InitAsDescriptorTable(1, &cloudTex1DescRange, D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[ShaderParams::t1Cloud2Tex].InitAsDescriptorTable(1, &cloudTex2DescRange, D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[ShaderParams::t2SkySB].InitAsDescriptorTable(1, &skySBDescRange, D3D12_SHADER_VISIBILITY_PIXEL);

		CD3DX12_STATIC_SAMPLER_DESC linearRepeatSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
		linearRepeatSampler.MaxAnisotropy = 1;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
		rootSignatureDescription.Init_1_1(ShaderParams::NumRootParameters, rootParameters, 1, &linearRepeatSampler, rootSignatureFlags);

		m_RootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, sdInfo->rootSignatureVersion);

		D3D12_DEPTH_STENCIL_DESC skyPlaneDSS;
		skyPlaneDSS.DepthEnable = false;
		skyPlaneDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		skyPlaneDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		skyPlaneDSS.StencilEnable = true;
		skyPlaneDSS.StencilReadMask = 0xff; // 255
		skyPlaneDSS.StencilWriteMask = 0xff;

		skyPlaneDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		skyPlaneDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_INCR;
		skyPlaneDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		skyPlaneDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

		skyPlaneDSS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		skyPlaneDSS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_DECR;
		skyPlaneDSS.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		skyPlaneDSS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

		D3D12_RENDER_TARGET_BLEND_DESC skyPlaneBlendDesc;
		skyPlaneBlendDesc.BlendEnable = true;
		skyPlaneBlendDesc.LogicOpEnable = false;
		skyPlaneBlendDesc.SrcBlend = D3D12_BLEND_ONE;
		skyPlaneBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		skyPlaneBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
		skyPlaneBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
		skyPlaneBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
		skyPlaneBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		skyPlaneBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
		skyPlaneBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPlanePsoStream;
		ZeroMemory(&skyPlanePsoStream, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		skyPlanePsoStream.pRootSignature = m_RootSignature.GetRootSignature().Get();
		skyPlanePsoStream.InputLayout = { core::PosNormTexVertex::InputElements, core::PosNormTexVertex::InputElementCount };
		skyPlanePsoStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		skyPlanePsoStream.VS = { g_BitmapClouds_VS, sizeof(g_BitmapClouds_VS) };
		skyPlanePsoStream.PS = { g_BitmapClouds_PS, sizeof(g_BitmapClouds_PS) };
		skyPlanePsoStream.SampleMask = UINT_MAX;
		skyPlanePsoStream.NumRenderTargets = 1;
		skyPlanePsoStream.SampleDesc.Count = 1;
		skyPlanePsoStream.SampleDesc.Quality = 0;
		skyPlanePsoStream.DSVFormat = sdInfo->depthBufFormat;
		skyPlanePsoStream.RTVFormats[0] = sdInfo->rtvFormats.RTFormats[0];
		skyPlanePsoStream.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		skyPlanePsoStream.BlendState.RenderTarget[0] = skyPlaneBlendDesc;
		skyPlanePsoStream.DepthStencilState = skyPlaneDSS;

		ThrowIfFailed(device->CreateGraphicsPipelineState(&skyPlanePsoStream, IID_PPV_ARGS(&m_PipelineState)));
	}
}

void BitmapClouds::OnUpdate(std::shared_ptr<CommandList>& commandList, UpdateEventArgs& e)
{
	if (!m_Camera.get())
		return;

	auto camPos = m_Camera->GetPosition();
	DirectX::XMMATRIX world = DirectX::XMMatrixTranslationFromVector(camPos);
	DirectX::XMMATRIX view = m_Camera->get_ViewMatrix();
	DirectX::XMMATRIX proj = m_Camera->get_ProjectionMatrix();

	m_MVP = world * view * proj;

	m_cloud1TexTranslationX += m_cloud1TexTranslationXSpeed;
	m_cloud1TexTranslationZ += m_cloud1TexTranslationZSpeed;
	m_cloud2TexTranslationX += m_cloud2TexTranslationXSpeed;
	m_cloud2TexTranslationZ += m_cloud2TexTranslationZSpeed;

	if (m_cloud1TexTranslationX > 1.0f) m_cloud1TexTranslationX = 0.f;
	if (m_cloud1TexTranslationZ > 1.0f) m_cloud1TexTranslationZ = 0.f;
	if (m_cloud2TexTranslationX > 1.0f) m_cloud2TexTranslationX = 0.f;
	if (m_cloud2TexTranslationZ > 1.0f) m_cloud2TexTranslationZ = 0.f;

	m_SkyBufferStruct[0] = { m_cloud1TexTranslationX, m_cloud1TexTranslationZ, m_cloud2TexTranslationX, m_cloud2TexTranslationZ, m_brightness };
	commandList->CopyStructuredBuffer(m_SkyBuffer, m_SkyBufferStruct);
}

void BitmapClouds::OnPreRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e)
{
}

void BitmapClouds::OnRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e)
{
	commandList->SetPipelineState(m_PipelineState);
	commandList->SetGraphicsRootSignature(m_RootSignature);

	commandList->SetGraphics32BitConstants(ShaderParams::b0MatCB, m_MVP);
	commandList->SetShaderResourceView(ShaderParams::t0Cloud1Tex, 0, m_CloudTex1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList->SetShaderResourceView(ShaderParams::t1Cloud2Tex, 0, m_CloudTex2, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList->SetShaderResourceView(ShaderParams::t2SkySB, 0, m_SkyBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	m_SkyPlaneMesh->Render(commandList);
}
