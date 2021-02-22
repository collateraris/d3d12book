#pragma once

#include <RenderPassBase.h>
#include <RootSignature.h>
#include <RenderTarget.h>
#include <StructuredBuffer.h>

#include <memory>

namespace dx12demo::core
{
	class CommandList;
	class Mesh;

	struct PrecomputeParticleDensityForAtmScatRenderPassInfo : public RenderPassBaseInfo
	{
		PrecomputeParticleDensityForAtmScatRenderPassInfo() {};

		virtual ~PrecomputeParticleDensityForAtmScatRenderPassInfo() = default;

		D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion;

		int width = 1024;
		int height = 1024;

	};

	class PrecomputeParticleDensityForAtmScatRP : public RenderPassBase
	{
	public:
		PrecomputeParticleDensityForAtmScatRP();

		virtual ~PrecomputeParticleDensityForAtmScatRP();

		virtual void LoadContent(RenderPassBaseInfo*) override;

		virtual void OnUpdate(std::shared_ptr<CommandList>& commandList, UpdateEventArgs& e) override;

		virtual void OnPreRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e) override;

		virtual void OnRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e) override;

		const Texture& GetPrecomputeParticleDensity() const;

		void BindAtmosphereParams(std::shared_ptr<CommandList>& commandList, StructuredBuffer& atmPar);

	private:

		D3D12_RECT m_ScissorRect;
		D3D12_VIEWPORT m_Viewport;

		RenderTarget m_RenderTarget;

		RootSignature m_RootSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;
	};
}