struct PixelShaderInput
{
    float3 TexCoord : TEXCOORD;
    float4 Position : SV_POSITION;
};

struct SkyBuffer
{
	float translation;
	float scale;
	float brightness;
	float padding;
};

Texture2D<float4> WeatherMapTex : register(t0);
Texture3D<float4> LowFreqNoiseTex : register(t1);
Texture3D<float4> HighFreqNoiseTex : register(t2);
StructuredBuffer<SkyBuffer> SkyDataSB : register(t3);

SamplerState LinearClampSampler : register(s0);

float4 main(PixelShaderInput IN) : SV_Target
{
	return HighFreqNoiseTex.Sample(LinearClampSampler, IN.TexCoord);
}