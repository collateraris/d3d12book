struct PixelShaderInput
{
    float3 TexCoord : TEXCOORD;
    float4 Position : SV_POSITION;
};

struct SkyBuffer
{
	float firstTranslationX;
	float firstTranslationZ;
	float secondTranslationX;
	float secondTranslationZ;
	float brightness;
	float3 padding;
};

Texture2D<float4> Cloud1Tex : register(t0);
Texture2D<float4> Cloud2Tex : register(t1);
StructuredBuffer<SkyBuffer> SkyDataSB : register(t2);

SamplerState LinearClampSampler : register(s0);

float4 main(PixelShaderInput IN) : SV_Target
{
	float3 samplerDelta = float3(0., 0., 0.);
	samplerDelta.x = IN.TexCoord.x + SkyDataSB[0].firstTranslationX;
	samplerDelta.y = IN.TexCoord.y + SkyDataSB[0].firstTranslationZ;

	float4 cloud1Color = Cloud1Tex.Sample(LinearClampSampler, samplerDelta);

	samplerDelta.x = IN.TexCoord.x + SkyDataSB[0].secondTranslationX;
	samplerDelta.y = IN.TexCoord.y + SkyDataSB[0].secondTranslationZ;

	float4 cloud2Color = Cloud2Tex.Sample(LinearClampSampler, samplerDelta);

	float4 finalColor = lerp(cloud1Color, cloud2Color, 0.5f);

	finalColor *= SkyDataSB[0].brightness;

	return finalColor;
}