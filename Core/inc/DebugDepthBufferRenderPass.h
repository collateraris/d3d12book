#pragma once

#include <RenderPassBase.h>
#include <RenderTarget.h>
#include <RootSignature.h>

namespace dx12demo::core
{
	class Mesh;
	class CommandList;

	struct DebugDepthBufferRenderPassInfo : public RenderPassBaseInfo
	{
		DebugDepthBufferRenderPassInfo() {};

		virtual ~DebugDepthBufferRenderPassInfo() = default;

		std::shared_ptr<CommandList> commandList;
		D3D12_RT_FORMAT_ARRAY rtvFormats;
		DXGI_FORMAT dsvFormat;
		D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion;
	};

	class DebugDepthBufferRenderPass : public RenderPassBase
	{
	public:

		DebugDepthBufferRenderPass();

		virtual ~DebugDepthBufferRenderPass();

		virtual void LoadContent(RenderPassBaseInfo*) override;

		virtual void OnUpdate(std::shared_ptr<CommandList>& commandList, UpdateEventArgs& e) override;
		virtual void OnPreRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e) override;
		virtual void OnRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e) override;


	private:

		std::unique_ptr<Mesh> m_QuadMesh;
		RootSignature m_RootSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;
	};
}