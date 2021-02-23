#include "CommonInclude.hlsl"

#define BLOCK_SIZE 16

struct VertexShaderOutput
{
    float2 TexCoord    : TEXCOORD;
    float4 Position    : SV_POSITION;
    float4 NormalVS    : NORMAL;
    //float3 TangentVS   : TANGENT;      // View space tangent.
    //float3 BinormalVS  : BINORMAL;     // View space binormal.
};

Texture2D DiffuseTexture  : register(t0);
Texture2D SpecularTexture : register(t1);
Texture2D NormalTexture   : register(t2);

SamplerState LinearRepeatSampler : register(s0);

struct PixelShaderOutput
{
    float4 albedoSpec : SV_TARGET0;
    float2 position : SV_TARGET1;
    float4 normal : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput IN) : SV_TARGET
{
    PixelShaderOutput output;

    float3 diffuse = DiffuseTexture.Sample(LinearRepeatSampler, IN.TexCoord).xyz;
    float specular = SpecularTexture.Sample(LinearRepeatSampler, IN.TexCoord).r;
    output.albedoSpec = float4(diffuse, specular);

    output.normal = normalize(IN.NormalVS);

    output.position = GetUV(IN.Position);

    return output;
}

