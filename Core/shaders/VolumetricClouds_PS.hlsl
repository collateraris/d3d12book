struct PixelShaderInput
{
	float3 domePosition : TEXCOORD;
	float4 Position : SV_POSITION;
};

struct SkyBuffer
{
	float translation;
	float scale;
	float brightness;
	float density;
	float coverage;
	float attenuation;
	float attenuation2;
	float sunIntensity;
};

static const int CLOUDS_MIN = 6415;
static const int CLOUDS_MAX = 6435;
static const int PLANET_RADIUS = 6400;
static const float3 SPHERE_ORIGIN = float3(0., 0., 0.);
static const float3 VIEW_POS = float3(0.0, 6400.0, 0.0);

Texture2D<float4> WeatherMapTex : register(t0);
Texture3D<float4> LowFreqNoiseTex : register(t1);
Texture3D<float4> HighFreqNoiseTex : register(t2);
StructuredBuffer<SkyBuffer> SkyDataSB : register(t3);

SamplerState LinearClampSampler : register(s0);

float4 mainMarching(in float3 viewDir, in float3 sunDir);
float cloudSampleDensity(in float3 position);
float cloudSampleDirectDensity(in float3 position, in float3 sunDir);
float cloudGetHeight(in float3 position);
float remap(float value, float minValue, float maxValue, float newMinValue, float newMaxValue);
bool crossRaySphereOutFar(in float3 ro, in float3 rd, in float sr, out float3 pos);

float4 main(PixelShaderInput IN) : SV_Target
{
	// Determine the position on the sky dome where this pixel is located.
	float height = IN.domePosition.y;

	// The value ranges from -1.0f to +1.0f so change it to only positive values.
	if (height < -0.07)
	{
		return float4(0.f, 0.f, 0.f, 0.0f);
	}

	float3 pos = IN.domePosition * CLOUDS_MIN;
	float3 rayDir = normalize(pos - VIEW_POS);
	float3 sunDir = float3(0., 1., 0.);
	return mainMarching(rayDir, sunDir);
}

float4 mainMarching(in float3 viewDir, in float3 sunDir)
{
	float3 position;
	if (crossRaySphereOutFar(VIEW_POS, viewDir, CLOUDS_MIN, position))
	{
		float avrStep = (CLOUDS_MAX - CLOUDS_MIN) / 64.0;

		float3 color = float3(0., 0., 0.);
		float transmittance = 1.0;

		//if (distance(position, SPHERE_ORIGIN) >= CLOUDS_MIN)
		//	return float4(0., 1., 1., 1.);

		[loop]
		for (int i = 0; i < 128; i++)
		{
			float density = cloudSampleDensity(position) * avrStep;
	
			float sunDensity = cloudSampleDirectDensity(position, sunDir);

			float m2 = exp(-SkyDataSB[0].attenuation * sunDensity);
			float m3 = SkyDataSB[0].attenuation2 * density;
			float light = SkyDataSB[0].sunIntensity * m2 * m3;

			color += light * transmittance;
			transmittance *= exp(-SkyDataSB[0].attenuation * density);
				
			position += viewDir * avrStep;
			if (length(position) > CLOUDS_MAX)
				break;
		}

		return float4(color, transmittance);
	}
	else
	{
		return float4(1., 0., 1., 1.);
	}
}

float cloudSampleDensity(in float3 position)
{
	position.xy += SkyDataSB[0].translation;

	float4 weather = WeatherMapTex.Sample(LinearClampSampler, position.xz / 4096.0f, 0);
	float height = cloudGetHeight(position);

	float SRb = saturate(remap(height, 0, 0.07, 0, 1));
	float SRt = saturate(remap(height, weather.b * 0.2, weather.b, 1, 0));
	float SA = SRb * SRt;

	float DRb = height * saturate(remap(height, 0, 0.15, 0, 1));
	float DRt = height * saturate(remap(height, 0.9, 1, 1, 0));
	float DA = DRb * DRt * weather.a * 2 * SkyDataSB[0].density;

	float SNsample = LowFreqNoiseTex.Sample(LinearClampSampler, position / 48.0f, 0).r * 0.85f + HighFreqNoiseTex.Sample(LinearClampSampler, position / 4.8f, 0).r * 0.15f;

	float WMc = max(weather.r, saturate(SkyDataSB[0].coverage - 0.5) * weather.g * 2);

	float d = saturate(remap(SNsample * SA, 1 - SkyDataSB[0].coverage * WMc, 1, 0, 1)) * DA;

	return d;
}

float cloudSampleDirectDensity(in float3 position, in float3 sunDir)
{
	float avrStep = (CLOUDS_MAX - CLOUDS_MIN) * 0.01;
	float sumDensity = 0.0;
	for (int i = 0; i < 4; i++)
	{
		float step = avrStep;
		if (i == 3)
			step = step * 6.0;
		position += sunDir * step;
		float density = cloudSampleDensity(position) * step;
		sumDensity += density;
	}
	return sumDensity;
}

float cloudGetHeight(in float3 position)
{
	float heightFraction = (distance(position, SPHERE_ORIGIN) - CLOUDS_MIN) / (CLOUDS_MAX - CLOUDS_MIN);
	return saturate(heightFraction);
}

float remap(float value, float minValue, float maxValue, float newMinValue, float newMaxValue)
{
	return newMinValue + (value - minValue) / (maxValue - minValue) * (newMaxValue - newMinValue);
}

bool crossRaySphereOutFar(in float3 ro, in float3 rd, in float sr, out float3 pos)
{
	float3 L = SPHERE_ORIGIN - ro;
	L = normalize(L);
	float tca = dot(L, rd);
	if (tca < 0.) return false;

	float sqD = dot(L, L) - tca * tca;
	float sqRadius = sr * sr;

	if (sqD > sqRadius) return false;

	float thc = sqrt(sqRadius - sqD);
	float delta = thc - tca;

	pos = ro + delta * rd;
	return true;
}