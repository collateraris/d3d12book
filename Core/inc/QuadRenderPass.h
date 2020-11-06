#pragma once

#include <RenderPassBase.h>
#include <RootSignature.h>

namespace dx12demo::core
{
	class CommandList;
	class Mesh;

	struct QuadRenderPassInfo : public RenderPassBaseInfo
	{
		QuadRenderPassInfo() {};

		virtual ~QuadRenderPassInfo() = default;

		D3D12_RT_FORMAT_ARRAY rtvFormats;
		D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion;

	};

	class QuadRenderPass : public RenderPassBase
	{
	public:
		QuadRenderPass();

		virtual ~QuadRenderPass();

		virtual void LoadContent(RenderPassBaseInfo*) override;

		virtual void OnUpdate(std::shared_ptr<CommandList>& commandList, UpdateEventArgs& e) override;

		virtual void OnPreRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e) override;

		virtual void OnRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e) override;

	private:

		RootSignature m_RootSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;

		std::unique_ptr<Mesh> m_QuadMesh;
	};
}