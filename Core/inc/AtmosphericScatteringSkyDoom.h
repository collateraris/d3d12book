#pragma once

#include <RenderPassBase.h>
#include <RootSignature.h>
#include <Mesh.h>
#include <DirectXMath.h>
#include <StructuredBuffer.h>
#include <Camera.h>

#include <memory>
#include <string>
#include <vector>

namespace dx12demo::core
{
	using namespace dx12demo::core;

	struct AtmosphericScatteringSkyDoomRenderPassInfo : public RenderPassBaseInfo
	{
		AtmosphericScatteringSkyDoomRenderPassInfo() {};

		virtual ~AtmosphericScatteringSkyDoomRenderPassInfo() = default;

		D3D12_RT_FORMAT_ARRAY rtvFormats;
		D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion;
		DXGI_FORMAT depthBufFormat;

		std::shared_ptr<Camera> camera;
	};

	class AtmosphericScatteringSkyDoom : public RenderPassBase
	{
		struct AtmospereData
		{
			DirectX::XMFLOAT3 viewPos;
		};
	public:

		AtmosphericScatteringSkyDoom();

		virtual ~AtmosphericScatteringSkyDoom();

		virtual void LoadContent(RenderPassBaseInfo*) override;

		virtual void OnUpdate(std::shared_ptr<CommandList>& commandList, UpdateEventArgs& e) override;

		virtual void OnPreRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e) override;

		virtual void OnRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e) override;

	private:

		std::shared_ptr<Mesh> m_SkyDoomMesh;

		AtmospereData m_AtmData;
		StructuredBuffer m_AtmDataBuffer;

		std::shared_ptr<Camera> m_Camera;

		RootSignature m_RootSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;

		DirectX::XMMATRIX m_MVP;
	};
}