#pragma once

#include <Light.h>
#include <config_sys/Config.h>
#include <StringConstant.h>

#include <vector>

namespace dx12demo::fpdu
{
	enum ELightParams
	{
		Position = 0,
		Direction = 1,
		Color = 2,
		SpotlightAngle = 3,
		Range = 4,
		Intensity = 5,
		Enabled = 6,
		Type = 7,
	};

	void CollectLightsFromConfig(core::Config& config, std::vector<Light>& lightList);

	void CollectLightsFromConfig(core::Config& config, std::vector<Light>& lightList)
	{
		auto lightsConfig = config.GetRoot().GetPath(LightsStr).GetChildren();
		for (auto& lightConfig : lightsConfig)
		{
			float x, y, z, w;
			Light light;
			auto item = lightConfig.GetChildren();
			if (item.empty())
				continue;
			auto pos = item[ELightParams::Position].GetChildren();
			x = pos[0].GetValueText<float>();
			y = pos[1].GetValueText<float>();
			z = pos[2].GetValueText<float>();
			w = pos[3].GetValueText<float>();
			light.m_PositionWS = { x, y, z, w };

			auto dir = item[ELightParams::Direction].GetChildren();
			x = dir[0].GetValueText<float>();
			y = dir[1].GetValueText<float>();
			z = dir[2].GetValueText<float>();
			w = dir[3].GetValueText<float>();
			light.m_DirectionWS = { x, y, z, w };

			auto color = item[ELightParams::Color].GetChildren();
			x = color[0].GetValueText<float>();
			y = color[1].GetValueText<float>();
			z = color[2].GetValueText<float>();
			w = color[3].GetValueText<float>();
			light.m_Color = { x, y, z, w };

			light.m_SpotlightAngle = item[ELightParams::SpotlightAngle].GetValueText<float>();
			light.m_Range = item[ELightParams::Range].GetValueText<float>();
			light.m_Intensity = item[ELightParams::Intensity].GetValueText<float>();
			light.m_Enabled = item[ELightParams::Enabled].GetValueText<uint32_t>();
			light.m_Type = static_cast<Light::ELightType>(item[ELightParams::Type].GetValueText<uint32_t>());

			lightList.emplace_back(light);
		}
	}
}