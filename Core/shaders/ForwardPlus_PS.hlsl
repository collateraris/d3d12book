#include "CommonInclude.hlsl"

#define BLOCK_SIZE 16

struct PixelShaderInput
{
    float4 PositionVS  : POSITION;
    float2 TexCoord    : TEXCOORD;
    float4 Position    : SV_Position;
    float3 NormalVS    : NORMAL;
    float3 TangentVS   : TANGENT;      // View space tangent.
    float3 BinormalVS  : BINORMAL;     // View space binormal.
};

Texture2D AmbientTexture  : register(t0);
Texture2D DiffuseTexture  : register(t1);
Texture2D SpecularTexture : register(t2);
Texture2D NormalTexture   : register(t3);
Texture2D<uint2> LightGrid : register(t4);

StructuredBuffer<Light> Lights : register(t5);
StructuredBuffer<uint>  LightIndexList : register(t6);

SamplerState LinearRepeatSampler : register(s0);

float4 main(PixelShaderInput IN) : SV_Target0
{
    const float4 eyePos = { 0, 0, 0, 1 };

    float4 ambient = AmbientTexture.Sample(LinearRepeatSampler, IN.TexCoord);
    float4 diffuse = DiffuseTexture.Sample(LinearRepeatSampler, IN.TexCoord);
    float4 specular = SpecularTexture.Sample(LinearRepeatSampler, IN.TexCoord);

    // for normal mapping
    float3x3 TBN = float3x3(normalize(IN.TangentVS),
                            normalize(IN.BinormalVS),
                            normalize(IN.NormalVS));
    float3 normal = NormalTexture.Sample(LinearRepeatSampler, IN.TexCoord).xyz;

    float3 N = DoNormalMapping(normal, TBN);
    float3 P = IN.PositionVS;
    float3 V = normalize(eyePos.xyz - P);

    // Get the index of the current pixel in the light grid.
    uint2 tileIndex = uint2(floor(IN.Position.xy / BLOCK_SIZE));

    // Get the start position and offset of the light in the light index list.
    uint2 tileData = LightGrid[tileIndex];
    uint startOffset = tileData.x;
    uint lightCount = tileData.y;

    /*
    LightResult lightsRes = (LightResult)0;
    
    for (uint i = 0; i < lightCount; ++i)
    {
        uint lightIndex = LightIndexList[startOffset + i];
        if (lightIndex <= 0)
            return float4(1., 0., 0., 1.);
        else
            return float4(0., 1., 0., 1.);
        Light light = Lights[lightIndex];

        /*
        // Skip lights that are not enabled.
        if (!light.Enabled || (light.Type != DIRECTIONAL_LIGHT && length(light.PositionVS - P) > light.Range))
            continue;

        LightResult result;

        switch (light.Type)
        {
        case DIRECTIONAL_LIGHT:
        {
            DoDirectionalLight(light, V, P, N, result);
        }
        break;
        case POINT_LIGHT:
        {
            DoPointLight(light, V, P, N, result);
        }
        break;
        case SPOT_LIGHT:
        {
            DoSpotLight(light, V, P, N, result);
        }
        break;
        }

        lightsRes.Diffuse += result.Diffuse;
        lightsRes.Specular += result.Specular;
        */
   // }
    

    //diffuse = lightsRes.Diffuse;
    //specular = lightsRes.Specular;
    
    //return float4((diffuse + specular).rgb, ambient.a);
    return float4((ambient + diffuse ).rgb, ambient.a);
}

