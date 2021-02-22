#pragma once

#include <RenderPassBase.h>
#include <RootSignature.h>
#include <Mesh.h>
#include <DirectXMath.h>
#include <Camera.h>

#include <memory>
#include <string>

namespace dx12demo::core
{
	using namespace dx12demo::core;

	struct SkyDoomCommonRenderPassInfo : public RenderPassBaseInfo
	{
		SkyDoomCommonRenderPassInfo() {};

		virtual ~SkyDoomCommonRenderPassInfo() = default;

		std::shared_ptr<RootSignature> rootSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;

		std::shared_ptr<Mesh> skydoomMesh;

		std::shared_ptr<Camera> camera;
	};

	class SkyDoomCommon : public RenderPassBase
	{
	public:

		SkyDoomCommon();

		virtual ~SkyDoomCommon();

		virtual void LoadContent(RenderPassBaseInfo*) override;

		virtual void OnUpdate(std::shared_ptr<CommandList>& commandList, UpdateEventArgs& e) override;

		virtual void OnPreRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e) override;

		virtual void OnRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e) override;

	private:

		std::shared_ptr<Mesh> m_SkyDoomMesh;

		std::shared_ptr<Camera> m_Camera;

		std::shared_ptr<RootSignature> m_RootSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;

		DirectX::XMMATRIX m_MVP;
	};

	class AtmosphericScatteringSkyDoom;

	class SkyDoomFabric : public URootObject
	{
	public:

		static std::shared_ptr<SkyDoomCommon> GetGradientType(std::shared_ptr<Camera>& camera, D3D12_RT_FORMAT_ARRAY rtvFormats,
			DXGI_FORMAT depthBufferFormat, D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion);

		static std::shared_ptr<AtmosphericScatteringSkyDoom> GetAtmosphericScatteringType(std::shared_ptr<Camera>& camera, D3D12_RT_FORMAT_ARRAY rtvFormats,
			DXGI_FORMAT depthBufferFormat, D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion);
	};
}