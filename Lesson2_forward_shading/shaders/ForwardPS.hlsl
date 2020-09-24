struct PixelShaderInput
{
    float4 PositionVS : POSITION;
    float3 NormalVS   : NORMAL;
    float2 TexCoord   : TEXCOORD;
    float3 TangentVS   : TANGENT;
    float3 BitangentVS : BITANGENT;
};

Texture2D DiffuseTexture : register(t0);

SamplerState LinearRepeatSampler : register(s0);

float4 main(PixelShaderInput IN) : SV_Target0
{
   return DiffuseTexture.Sample(LinearRepeatSampler, IN.TexCoord);
}