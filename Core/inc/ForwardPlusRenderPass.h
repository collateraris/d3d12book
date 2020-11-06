#pragma once

#include <RenderPassBase.h>
#include <RenderPassBase.h>
#include <RootSignature.h>

namespace dx12demo::core
{
	class Texture;
	class StructuredBuffer;

	struct ForwardPlusRenderPassInfo : public RenderPassBaseInfo
	{
		ForwardPlusRenderPassInfo() {};

		virtual ~ForwardPlusRenderPassInfo() = default;

		D3D12_RT_FORMAT_ARRAY rtvFormats;
		DXGI_FORMAT depthBufferFormat;
		D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion;

	};

	class ForwardPlusRenderPass : public RenderPassBase
	{
	public:

		ForwardPlusRenderPass();

		virtual ~ForwardPlusRenderPass();

		virtual void LoadContent(RenderPassBaseInfo*) override;

		virtual void OnUpdate(std::shared_ptr<CommandList>& commandList, UpdateEventArgs& e) override;

		virtual void OnPreRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e) override;

		virtual void OnRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e) override;

		void AttachAmbientTex(std::shared_ptr<CommandList>& commandList, const Texture& tex);
		void AttachDiffuseTex(std::shared_ptr<CommandList>& commandList, const Texture& tex);
		void AttachSpecularTex(std::shared_ptr<CommandList>& commandList, const Texture& tex);
		void AttachNormalTex(std::shared_ptr<CommandList>& commandList, const Texture& tex);

		void AttachLightGridTex(std::shared_ptr<CommandList>& commandList, const Texture& tex);
		void AttachLightsSB(std::shared_ptr<CommandList>& commandList, const StructuredBuffer& sb);
		void AttachLightIndexListSB(std::shared_ptr<CommandList>& commandList, const StructuredBuffer& sb);

	private:

		RootSignature m_RootSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;
	};
}