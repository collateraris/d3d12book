#pragma once

#include <RenderPassBase.h>
#include <RootSignature.h>
#include <RenderTarget.h>
#include <Texture.h>

namespace dx12demo::core
{
	class Texture;
	class StructuredBuffer;

	enum EGBuffer
	{
		G_AlbedoSpecular = 0,
		G_Normal,
		G_MAX,
	};

	struct DefferedRenderPassInfo : public RenderPassBaseInfo
	{

		DefferedRenderPassInfo()
		{
			for (int i = 0; i < 8; ++i)
				rtvFormats.RTFormats[i] = DXGI_FORMAT_UNKNOWN;

			rtvFormats.NumRenderTargets = EGBuffer::G_MAX;
			rtvFormats.RTFormats[EGBuffer::G_AlbedoSpecular] = DXGI_FORMAT_R16G16B16A16_FLOAT;
			rtvFormats.RTFormats[EGBuffer::G_Normal] = DXGI_FORMAT_R8G8B8A8_SNORM;

			depthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		}

		virtual ~DefferedRenderPassInfo() = default;
		UINT m_Width, m_Height;
		D3D12_RT_FORMAT_ARRAY rtvFormats;
		DXGI_FORMAT depthBufferFormat;
		D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion;

	};

	class DefferedRenderPass : public RenderPassBase
	{
	public:

		DefferedRenderPass();

		virtual ~DefferedRenderPass();

		virtual void LoadContent(RenderPassBaseInfo*) override;

		virtual void OnUpdate(std::shared_ptr<CommandList>& commandList, UpdateEventArgs& e) override;

		virtual void OnPreRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e) override;

		virtual void OnRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e) override;

		void AttachDiffuseTex(std::shared_ptr<CommandList>& commandList, const Texture& tex);
		void AttachSpecularTex(std::shared_ptr<CommandList>& commandList, const Texture& tex);
		void AttachNormalTex(std::shared_ptr<CommandList>& commandList, const Texture& tex);

		const Texture& GetDepthBuffer() const;
		const Texture& GetGBuffer(EGBuffer tex) const;
		const D3D12_SHADER_RESOURCE_VIEW_DESC& GetDepthBufferSRV() const;

	private:

		RootSignature m_RootSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;
		RenderTarget m_GBuffer;
		D3D12_RECT m_ScissorRect;
		D3D12_SHADER_RESOURCE_VIEW_DESC m_DepthBufferSRV;

	};
}