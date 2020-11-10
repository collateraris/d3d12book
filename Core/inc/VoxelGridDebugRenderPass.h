#pragma once

#include <RenderPassBase.h>
#include <RootSignature.h>
#include <StructuredBuffer.h>
#include <Texture.h>
#include <VoxelGrid.h>

namespace dx12demo::core
{
	struct VoxelGridDebugRenderPassInfo : public RenderPassBaseInfo
	{
		VoxelGridDebugRenderPassInfo() {};

		virtual ~VoxelGridDebugRenderPassInfo() = default;

		D3D12_RT_FORMAT_ARRAY rtvFormats;
		D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion;

	};

	class CommandList;

	class VoxelGridDebugRenderPass : RenderPassBase
	{
		struct Mat
		{
			DirectX::XMMATRIX invViewProj;
		};
	public:
		VoxelGridDebugRenderPass();
		virtual ~VoxelGridDebugRenderPass();

		virtual void LoadContent(RenderPassBaseInfo*) override;

		virtual void OnUpdate(std::shared_ptr<CommandList>& commandList, UpdateEventArgs& e) override;

		virtual void OnPreRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e) override;

		virtual void OnRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e) override;

		void AttachDepthTex(std::shared_ptr<CommandList>& commandList, const Texture& depthTex);
		void AttachVoxelGrid(std::shared_ptr<CommandList>& commandList, const StructuredBuffer& sb);
		void AttachVoxelGridParams(std::shared_ptr<CommandList>& commandList, const VoxelGrid& voxelGrid);
		void AttachInvViewProjMatrix(std::shared_ptr<CommandList>& commandList, const DirectX::XMMATRIX& invViewProj);

	private:

		RootSignature m_DebugRootSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_DebugPipelineState;

		Mat m_Mat;
	};
}