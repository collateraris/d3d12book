#pragma once

#include <RenderPassBase.h>
#include <RenderTarget.h>
#include <RootSignature.h>

namespace dx12demo::core
{
	struct DepthBufferRenderPassInfo : public RenderPassBaseInfo
	{
		DepthBufferRenderPassInfo() {};

		virtual ~DepthBufferRenderPassInfo() = default;

		int bufferW;
		int bufferH;
		D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion;

		bool bUseCustomDepthBufferDesc = false;

		D3D12_RESOURCE_DESC resDesc;
		D3D12_CLEAR_VALUE clearValue;
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	};

	class DepthBufferRenderPass : public RenderPassBase
	{
	public:

		DepthBufferRenderPass();

		virtual ~DepthBufferRenderPass();

		virtual void LoadContent(RenderPassBaseInfo*) override;

		virtual void OnUpdate(std::shared_ptr<CommandList>& commandList, UpdateEventArgs& e) override;

		virtual void OnRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e) override;

		const Texture& GetDepthBuffer() const;

		const D3D12_SHADER_RESOURCE_VIEW_DESC& GetSRV() const;

	private:

		D3D12_RECT m_ScissorRect;
		D3D12_VIEWPORT m_Viewport;

		RenderTarget m_RenderTarget;
		D3D12_SHADER_RESOURCE_VIEW_DESC m_SRV;

		RootSignature m_RootSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;
	};
}