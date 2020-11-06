Texture2D<float3> Texture : register(t0);
SamplerState LinearClampSampler : register(s0);

float4 main(float2 TexCoord : TEXCOORD) : SV_Target0
{
	return float4(Texture.Sample(LinearClampSampler, TexCoord), 1.f);
}