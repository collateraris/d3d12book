#pragma once

#include <URootObject.h>
#include <RootSignature.h>
#include <CommandList.h>
#include <Texture.h>
#include <StructuredBuffer.h>

#include <bitset>

namespace dx12demo::core
{
	class PrecomputeSkyboxLUTForAtmScat : URootObject
	{
	public:

		PrecomputeSkyboxLUTForAtmScat();

		virtual ~PrecomputeSkyboxLUTForAtmScat();

		void StartCompute(std::shared_ptr<CommandList>& commandList);

		void BindPrecomputeParticleDensity(std::shared_ptr<CommandList>& commandList, const Texture& partcleDens);
		void BindAtmosphereParams(std::shared_ptr<CommandList>& commandList, const StructuredBuffer& atmPar);

		void Compute(std::shared_ptr<CommandList>& commandList);

		const Texture& Get_RS_SkyboxLUT3DTex() const;
		const Texture& Get_MS_SkyboxLUT3DTex() const;

	protected:

		RootSignature m_RootSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;

		Texture m_RS_SkyboxLUT;
		Texture m_MS_SkyboxLUT;

		//state
		enum EBitsetStates
		{
			bIdle = 0,
			bStartCompute = 1,
			bBindPrecomputeParticleDensity,
			bBindAtmPar,
		};

		std::bitset<4> m_CurrState;
	};
}