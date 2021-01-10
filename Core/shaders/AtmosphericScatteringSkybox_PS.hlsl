#include "AtmosphericScatteringInclude.hlsl"

struct PixelShaderInput
{
    // Skybox texture coordinate
    float3 LocalPos : TEXCOORD;
};

struct ExternalData
{
    float3 lightPos;
    float3 camPos;
};

Texture3D<float4> tRS_SkyboxLUT3DTex : register(t0);
Texture3D<float2> tMS_SkyboxLUT3DTex : register(t1);
StructuredBuffer<AtmosphericScatteringParams> tAtmParSB : register(t2);
StructuredBuffer<ExternalData> tExtData : register(t3);

float4 main(PixelShaderInput IN) : SV_Target
{
    float3 rayOrigin = tExtData[0].camPos;
    float3 rayDir = normalize(IN.LocalPos);

    float3 lightDir = tExtData[0].lightPos;
    float3 planetCenter = float3(0., -tAtmParSB[0]._PlanetRadius, 0);

    float3 scatterR = 0;
    float3 scatterM = 0;

    float height = length(rayOrigin - planetCenter) - tAtmParSB[0]._PlanetRadius;
    float3 normal = normalize(rayOrigin - planetCenter);

    float viewZenith = dot(normal, rayDir);
    float sunZenith = dot(normal, -lightDir);

    float4 coords = 0;
    coords.x = sqrt(height / tAtmParSB[0]._AtmosphereHeight);
    float ch = -(sqrt(height * (2 * tAtmParSB[0]._PlanetRadius + height)) / (tAtmParSB[0]._PlanetRadius + height));
    if (viewZenith > ch)
    {
        coords.y = 0.5 * pow((viewZenith - ch) / (1 - ch), 0.2) + 0.5;
    }
    else
    {
        coords.y = 0.5 * pow((ch - viewZenith) / (ch + 1), 0.2);
    }
    coords.z = 0.5 * ((atan(max(sunZenith, -0.1975) * tan(1.26 * 1.1)) / 1.1) + (1 - 0.26));

    float4 rs_skybox = tRS_SkyboxLUT3DTex.Load(coords);
    scatterR = rs_skybox.xyz;
    scatterM.x = rs_skybox.w;
    scatterM.yz = tMS_SkyboxLUT3DTex.Load(coords).xy;

    float3 m = scatterM;

    float RdLd = dot(rayDir, -lightDir.xyz);
    scatterR = ApplyPhaseFunctionRiel(scatterR, RdLd);
    scatterM = ApplyPhaseFunctionMie(scatterM, RdLd, tAtmParSB[0]._MieG);

    float3 lightInscatter = (scatterR * tAtmParSB[0]._ScatteringR.xyz + scatterM * tAtmParSB[0]._ScatteringM.xyz);

    lightInscatter += RenderSun(m, RdLd) * tAtmParSB[0]._SunIntensity * float3(4., 4., 4.);

    return float4(max(0, lightInscatter), 1);
}