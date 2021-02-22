#pragma once

#include <RenderPassBase.h>
#include <RootSignature.h>
#include <DirectXMath.h>
#include <CommandList.h>
#include <StructuredBuffer.h>
#include <Texture.h>
#include <Mesh.h>
#include <Camera.h>

#include <PrecomputeParticleDensityForAtmScat.h>
#include <PrecomputeSkyboxLUTForAtmScat.h>

#include <string>
#include <memory>

namespace dx12demo::core
{
	struct AtmosphericScatteringSkyboxRPInfo : public RenderPassBaseInfo
	{
		AtmosphericScatteringSkyboxRPInfo() {};

		virtual ~AtmosphericScatteringSkyboxRPInfo() = default;

		std::shared_ptr<CommandList> commandList;
		std::shared_ptr<Camera> camera = nullptr;

		D3D12_RT_FORMAT_ARRAY rtvFormats;
		D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion;
	};

	class AtmosphericScatteringParamsWrapperSB
	{
		struct AtmosphericScatteringParams
		{
			float _AtmosphereHeight = 80000.0f;
			float _PlanetRadius = 6371000.0f;
			float _MieG = 0.76f;
			float _DistanceScale = 1;

			DirectX::XMFLOAT4 _ScatteringR;
			DirectX::XMFLOAT4 _ScatteringM;

			DirectX::XMFLOAT4 _ExtinctionR;
			DirectX::XMFLOAT4 _ExtinctionM;

			DirectX::XMFLOAT2 _DensityScaleHeight = { 7994.0f, 1200.0f};
			float _SunIntensity = 1;
			float  padding;
		};

		float m_RayleighScatterCoef = 1;
		float m_RayleighExtinctionCoef = 1;
		float m_MieScatterCoef = 1;
		float m_MieExtinctionCoef = 1;

		DirectX::XMFLOAT4 m_RayleighSct = { 5.8f * 0.000001f, 13.5f * 0.000001f, 33.1f * 0.000001f, 0.0f };
		DirectX::XMFLOAT4 m_MieSct = { 2.0f * 0.000001f, 2.0f * 0.000001f, 2.0f * 0.000001f, 0.0f };

		AtmosphericScatteringParams m_Params;
		StructuredBuffer m_ParamsSB;

		bool bIsNeedUpdate = true;

	public:

		AtmosphericScatteringParamsWrapperSB();
		~AtmosphericScatteringParamsWrapperSB();

		bool IsNeedUpdateAtmParams() const;

		void UpdateAtmParams(std::shared_ptr<CommandList>& commandList);

		const StructuredBuffer& GetAtmParamsSB() const;

		void Set_AtmosphereHeight(float);
		void Set_PlanetRadius(float);
		void Set_MieG(float);
		void Set_DistanceScale(float);

		void Set__DensityScaleHeight(const DirectX::XMFLOAT2&);
		void Set__SunIntensity(float);

		void SetRayleighScatterCoef(float);
		void SetRayleighExtinctionCoef(float);
		void SetMieScatterCoef(float);
		void SetMieExtinctionCoef(float);
	};

	class AtmosphericScatteringSkyboxRP : public RenderPassBase, public AtmosphericScatteringParamsWrapperSB
	{
		struct ExternalData
		{
			DirectX::XMFLOAT3 lightPos = {0., 1., 0.};
			DirectX::XMFLOAT3 camPos;
		};
	public:

		AtmosphericScatteringSkyboxRP();

		virtual ~AtmosphericScatteringSkyboxRP();

		virtual void LoadContent(RenderPassBaseInfo*) override;

		virtual void OnUpdate(std::shared_ptr<CommandList>&, UpdateEventArgs& e) override;

		virtual void OnRender(std::shared_ptr<CommandList>&, RenderEventArgs& e) override;

		void BindLightPos(const DirectX::XMFLOAT3& lightPos);

	protected:

		std::unique_ptr<Mesh> m_SkyboxMesh;

		RootSignature m_SkyboxSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_SkyboxPipelineState;

		PrecomputeParticleDensityForAtmScatRP m_PrecomputeParticleDensityRP;
		PrecomputeSkyboxLUTForAtmScat m_PrecomputeSkyboxLUTCRP;

		ExternalData m_ExtData;
		StructuredBuffer m_ExtDataSB;

		std::shared_ptr<Camera> m_Camera = nullptr;

		DirectX::XMMATRIX m_ViewProjMatrix;
	};
}