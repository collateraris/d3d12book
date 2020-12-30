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

Texture2D<float4> CloudTex : register(t0);
Texture2D<float4> NoiseTex : register(t1);
StructuredBuffer<SkyBuffer> SkyDataSB : register(t2);

SamplerState LinearClampSampler : register(s0);

float4 main(PixelShaderInput IN) : SV_Target
{
	IN.TexCoord.x = IN.TexCoord.x + SkyDataSB[0].translation;

	float4 noiseVal = NoiseTex.Sample(LinearClampSampler, IN.TexCoord);
	noiseVal *= SkyDataSB[0].scale;
	noiseVal.xy = noiseVal.xy + IN.TexCoord.xy + SkyDataSB[0].translation;

	float4 cloudColor = CloudTex.Sample(LinearClampSampler, noiseVal.xyz);

	cloudColor *= SkyDataSB[0].brightness;

	return cloudColor;
}