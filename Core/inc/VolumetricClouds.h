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

	struct VolumetricCloudsRenderPassInfo : public RenderPassBaseInfo
	{
		VolumetricCloudsRenderPassInfo() {};

		virtual ~VolumetricCloudsRenderPassInfo() = default;

		D3D12_RT_FORMAT_ARRAY rtvFormats;
		D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion;
		DXGI_FORMAT depthBufFormat;

		std::shared_ptr<Camera> camera;

		std::wstring weathermap_path = L"Assets/Textures/bitmap_clouds/weather_map.png";

		int skyPlaneResolution = 10;
		float skyPlaneWidth = 10.0f;
		float skyPlaneTop = 0.5f;
		float skyPlaneBottom = 0.0f;
		int textureRepeat = 4;
	};

	class VolumetricClouds : public RenderPassBase
	{

		struct SkyBuffer
		{
			float translation;
			float scale;
			float brightness;
			float padding;
		};

	public:

		VolumetricClouds();

		virtual ~VolumetricClouds();

		virtual void LoadContent(RenderPassBaseInfo*) override;

		virtual void OnUpdate(std::shared_ptr<CommandList>& commandList, UpdateEventArgs& e) override;

		virtual void OnPreRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e) override;

		virtual void OnRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e) override;

	private:

		std::unique_ptr<Mesh> m_SkyPlaneMesh;
		Texture m_WeatherMapTex;
		Texture m_LowFrequency3DTex;
		Texture m_HighFrequency3DTex;
		StructuredBuffer m_SkyBuffer;
		std::vector<SkyBuffer> m_SkyBufferStruct = { SkyBuffer{} };

		std::shared_ptr<Camera> m_Camera;

		RootSignature m_RootSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;

		DirectX::XMMATRIX m_MVP;

		float m_scale = 0.3f;
		float m_brightness = 0.5f;
		float m_translation = 0.0f;
		float m_translationSpeed = 0.0001f;

	};

}