#include "AtmosphericScatteringInclude.hlsl"

StructuredBuffer<AtmosphericScatteringParams> tAtmParSB : register(t0);

float2 main(float2 TexCoord : TEXCOORD) : SV_Target0
{
	float cos = TexCoord.x * 2.0 - 1.;
	float sin = sqrt(saturate(1. - cos * cos));
	float startHeight = lerp(0., tAtmParSB[0]._AtmosphereHeight, TexCoord.y);
	//float startHeight = lerp(0., 80000.0f, TexCoord.y);

	float3 rayOrigin = float3(0., startHeight, 0.);
	float3 rayDir = float3(sin, cos, 0.);

	//return PrecomputeParticleDensity(rayOrigin, rayDir, 6371000.0f, 80000.0f, float4(7994.0f, 1200.0f, 0, 0));
	return PrecomputeParticleDensity(rayOrigin, rayDir, tAtmParSB[0]._PlanetRadius, tAtmParSB[0]._AtmosphereHeight, tAtmParSB[0]._DensityScaleHeight);
}