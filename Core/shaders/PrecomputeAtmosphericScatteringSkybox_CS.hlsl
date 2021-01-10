#include "AtmosphericScatteringInclude.hlsl"

// View space frustums for the grid cells.
RWTexture3D<float4> uRS_SkyboxLUT3DTex : register(u0);
RWTexture3D<float2> uMS_SkyboxLUT3DTex : register(u1);

Texture2D<float2> tParticleDensityLUT : register(t0);
StructuredBuffer<AtmosphericScatteringParams> tAtmParSB : register(t1);

SamplerState LinearClampSampler : register(s0);

void GetAtmosphereDensity(float3 position, float3 planetCenter, float3 lightDir, AtmosphericScatteringParams par, out float2 localDensity, out float2 densityToAtmTop);

ScatteringOutput IntegrateInscattering(float3 rayStart, float3 rayDir, float rayLength, float3 planetCenter, float3 lightDir);

[numthreads(1, 1, 1)]
void main(ComputeShaderInput IN)
{
	float w, h, d;
	uRS_SkyboxLUT3DTex.GetDimensions(w, h, d);

	uint3 id = IN.dispatchThreadID;
	float3 coords = float3(id.x / (w - 1), id.y / (h - 1), id.z / (d - 1));

	float height = coords.x * coords.x * tAtmParSB[0]._AtmosphereHeight;

	float ch = -(sqrt(height * (2 * tAtmParSB[0]._PlanetRadius + height)) / (tAtmParSB[0]._PlanetRadius + height));

	float viewZenithAngle = coords.y;

	if (viewZenithAngle > 0.5)
	{
		viewZenithAngle = ch + pow((viewZenithAngle - 0.5) * 2, 5) * (1 - ch);
	}
	else
	{
		viewZenithAngle = ch - pow(viewZenithAngle * 2, 5) * (1 + ch);
	}

	float sunZenithAngle = (tan((2 * coords.z - 1 + 0.26) * 0.75)) / (tan(1.26 * 0.75));

	float3 planetCenter = float3(0, -tAtmParSB[0]._PlanetRadius, 0);
	float3 rayStart = float3(0, height, 0);

	float3 rayDir = float3(sqrt(saturate(1 - viewZenithAngle * viewZenithAngle)), viewZenithAngle, 0);
	float3 lightDir = -float3(sqrt(saturate(1 - sunZenithAngle * sunZenithAngle)), sunZenithAngle, 0);

	float rayLength = 1e20;
	float2 intersection = RaySphereIntersection(rayStart, rayDir, planetCenter, tAtmParSB[0]._PlanetRadius + tAtmParSB[0]._AtmosphereHeight);
	rayLength = intersection.y;

	intersection = RaySphereIntersection(rayStart, rayDir, planetCenter, tAtmParSB[0]._PlanetRadius);
	if (intersection.x > 0)
		rayLength = min(rayLength, intersection.x);

	ScatteringOutput scattering = IntegrateInscattering(rayStart, rayDir, rayLength, planetCenter, lightDir);

	uRS_SkyboxLUT3DTex[id.xyz] = float4(scattering.rayleigh.xyz, scattering.mie.x);
	uMS_SkyboxLUT3DTex[id.xyz] = scattering.mie.yz;
}

void GetAtmosphereDensity(float3 position, float3 planetCenter, float3 lightDir, out float2 localDensity, out float2 densityToAtmTop)
{
	float height = length(position - planetCenter) - tAtmParSB[0]._PlanetRadius;
	localDensity = exp(-height.xx / tAtmParSB[0]._DensityScaleHeight.xy);

	float sunZenit = dot(normalize(position - planetCenter), -lightDir);
	sunZenit = sunZenit * 0.5 + 0.5;

	densityToAtmTop = tParticleDensityLUT.SampleLevel(LinearClampSampler, float2(sunZenit, height / tAtmParSB[0]._AtmosphereHeight), 0.0).xy;
}

ScatteringOutput IntegrateInscattering(float3 rayStart, float3 rayDir, float rayLength, float3 planetCenter, float3 lightDir)
{
	float sampleCount = 128;
	float3 step = rayDir * (rayLength / sampleCount);
	float stepSize = length(step) * 0.5;

	float2 densityCP = 0;
	float2 densityPA;
	float3 scatterR = 0;
	float3 scatterM = 0;

	float2 localDensity;

	float2 prevLocalDensity;
	float3 prevLocalInscatterR, prevLocalInscatterM;

	GetAtmosphereDensity(rayStart, planetCenter, lightDir, prevLocalDensity, densityPA);
	ComputeLocalInscattering(prevLocalDensity, densityPA, densityCP, tAtmParSB[0]._ExtinctionR, tAtmParSB[0]._ExtinctionM, prevLocalInscatterR, prevLocalInscatterM);

	// P - current integration point
	// C - camera position
	// A - top of the atmosphere
	[loop]
	for (float s = 1.0; s < sampleCount; s += 1)
	{
		float3 p = rayStart + step * s;

		GetAtmosphereDensity(p, planetCenter, lightDir, localDensity, densityPA);
		densityCP += (localDensity + prevLocalDensity) * stepSize;

		prevLocalDensity = localDensity;

		float3 localInscatterR, localInscatterM;
		ComputeLocalInscattering(localDensity, densityPA, densityCP, tAtmParSB[0]._ExtinctionR, tAtmParSB[0]._ExtinctionM, localInscatterR, localInscatterM);

		scatterR += (localInscatterR + prevLocalInscatterR) * stepSize;
		scatterM += (localInscatterM + prevLocalInscatterM) * stepSize;

		prevLocalInscatterR = localInscatterR;
		prevLocalInscatterM = localInscatterM;
	}

	ScatteringOutput output;
	output.rayleigh = scatterR;
	output.mie = scatterM;

	return output;
}