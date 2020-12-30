#pragma once

#include <RenderPassBase.h>
#include <RootSignature.h>
#include <Mesh.h>
#include <DirectXMath.h>
#include <Camera.h>
#include <Texture.h>
#include <StructuredBuffer.h>

#include <memory>
#include <string>

namespace dx12demo::core
{
	using namespace dx12demo::core;

	struct BitmapCloudsRenderPassInfo : public RenderPassBaseInfo
	{
		BitmapCloudsRenderPassInfo() {};

		virtual ~BitmapCloudsRenderPassInfo() = default;

		D3D12_RT_FORMAT_ARRAY rtvFormats;
		D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion;
		DXGI_FORMAT depthBufFormat;

		std::shared_ptr<Camera> camera;

		std::wstring cloudtex1_path = L"Assets/Textures/bitmap_clouds/cloud001.dds";
		std::wstring cloudtex2_path = L"Assets/Textures/bitmap_clouds/cloud002.dds";

		int skyPlaneResolution = 10;
		float skyPlaneWidth = 10.0f;
		float skyPlaneTop = 0.5f;
		float skyPlaneBottom = 0.0f;
		int textureRepeat = 4;
	};

	class BitmapClouds : public RenderPassBase
	{

		struct SkyBuffer
		{
			float firstTranslationX;
			float firstTranslationZ;
			float secondTranslationX;
			float secondTranslationZ;
			float brightness;
			DirectX::XMFLOAT3 padding;
		};

	public:

		BitmapClouds();

		virtual ~BitmapClouds();

		virtual void LoadContent(RenderPassBaseInfo*) override;

		virtual void OnUpdate(std::shared_ptr<CommandList>& commandList, UpdateEventArgs& e) override;

		virtual void OnPreRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e) override;

		virtual void OnRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e) override;

	private:

		std::unique_ptr<Mesh> m_SkyPlaneMesh;
		Texture m_CloudTex1;
		Texture m_CloudTex2;
		StructuredBuffer m_SkyBuffer;
		std::vector<SkyBuffer> m_SkyBufferStruct = { SkyBuffer{} };

		std::shared_ptr<Camera> m_Camera;

		RootSignature m_RootSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;

		DirectX::XMMATRIX m_MVP;

		// Setup the cloud translation speed increments.
		float m_cloud1TexTranslationXSpeed = 0.0003f;
		float m_cloud1TexTranslationZSpeed = 0.0f;
		float m_cloud2TexTranslationXSpeed = 0.00015f;
		float m_cloud2TexTranslationZSpeed = 0.0f;

		// Initialize the texture translation values.
		float m_cloud1TexTranslationX = 0.0f;
		float m_cloud1TexTranslationZ = 0.0f;
		float m_cloud2TexTranslationX = 0.0f;
		float m_cloud2TexTranslationZ = 0.0f;

		float m_brightness = 0.65f;
	};

}