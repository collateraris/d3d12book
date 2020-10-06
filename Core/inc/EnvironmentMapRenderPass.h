#pragma once

#include <RenderPassBase.h>
#include <RootSignature.h>
#include <DirectXMath.h>
#include <CommandList.h>
#include <Texture.h>
#include <Mesh.h>
#include <Camera.h>

#include <string>
#include <memory>

namespace dx12demo::core
{
	struct EnvironmentMapRenderPassInfo : public RenderPassBaseInfo
	{
		EnvironmentMapRenderPassInfo() {};

		virtual ~EnvironmentMapRenderPassInfo() = default;

		CommandList* commandList = nullptr;
		Camera* camera = nullptr;

		std::wstring texturePath = L"";
		std::wstring textureName = L"";

		int16_t cubeMapSize = 0;
		int16_t cubeMapDepthOrArraySize = 0;
		int16_t cubeMapMipLevels = 0;

		D3D12_RT_FORMAT_ARRAY rtvFormats;
		D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion;
	};

	class EnvironmentMapRenderPass : public RenderPassBase
	{
	public:

		EnvironmentMapRenderPass();

		virtual ~EnvironmentMapRenderPass();

		virtual void LoadContent(RenderPassBaseInfo*) override;

		virtual void OnUpdate(CommandList&, UpdateEventArgs& e) override;

		virtual void OnRender(CommandList&, RenderEventArgs& e) override;

	protected:

		std::unique_ptr<Mesh> m_SkyboxMesh;

		RootSignature m_SkyboxSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_SkyboxPipelineState;

		Texture m_SrcTexture;
		Texture m_CubemapTexture;

		Camera* m_Camera = nullptr;

		DirectX::XMMATRIX m_ViewProjMatrix;

		D3D12_SHADER_RESOURCE_VIEW_DESC m_SrvDesc;
	};
}