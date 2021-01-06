struct PixelShaderInput
{
    // Skybox texture coordinate
    float3 domePosition : TEXCOORD;
	float4 Position : SV_POSITION;
};

struct AtmospereData
{
    float3 viewPos;
};

#define M_PI 3.1415926535897932384626433832795

static const float rPlanet = 6371e3;
static const float rAtmos = 6471e3;

StructuredBuffer<AtmospereData> AtmDataSB : register(t0);

float2 rsi(in float3 r0, in float3 rd, in float sr);

float3 atmosphere(in float3 rayDir, in float3 rayOrigin, in float3 sunPos, float sunIntensity);

float4 main(PixelShaderInput IN) : SV_Target
{
    // Determine the position on the sky dome where this pixel is located.
    float height = IN.domePosition.y;

	// The value ranges from -1.0f to +1.0f so change it to only positive values.
	if (height < -0.07)
	{
		return float4(0.81f, 0.38f, 0.66f, 1.0f);
	}

	float3 uSunPos = float3(0.7, 0.7, 0);
	float3 rayOrigin = float3(0, 6371e3, 0);
	float3 pos = IN.domePosition * rAtmos;
	float3 rayDir = normalize(pos - rayOrigin);

	// Determine the gradient color by interpolating between the apex and center based on the height of the pixel in the sky dome.
	return float4(atmosphere(rayDir, rayOrigin, uSunPos, 22.0), 1.0f);
}

float2 rsi(in float3 r0, in float3 rd, in float sr)
{
    float a = dot(rd, rd);
    float b = 2.0 * dot(rd, r0);
    float c = dot(r0, r0) - (sr * sr);
    float d = (b * b) - 4.0 * a * c;
    if (d < 0.0) return float2(1e5, -1e5);
    return float2(
        (-b - sqrt(d)) / (2.0 * a),
        (-b + sqrt(d)) / (2.0 * a)
    );
}

float3 atmosphere(in float3 rayDir, in float3 rayOrigin, in float3 sunPos, float sunIntensity)
{

	float tA, tB;
	float2 p = rsi(rayOrigin, rayDir, rAtmos);
	if (p.x > p.y) return float3(0, 0, 0);
	p.y = min(p.y, rsi(rayOrigin, rayDir, rPlanet).x);
	tA = p.x;
	tB = p.y;

	const int iSteps = 16;
	const int jSteps = 16;
	const float inviSteps = 0.0625;
	const float invjSteps = 0.0625;
	const float3 kRlh = float3(5.8e-6, 13.5e-6, 33.1e-6);
	const float3 kMie = float3(3e-6, 3e-6, 3e-6);
	const float shRlh = 8e3;
	const float shMie = 1.2e3;
	const float invshRlh = 1. / shRlh;
	const float invshMie = 1. / shMie;
	const float gMie = 0.758;
	float mu = dot(rayDir, sunPos);
	float mumu = mu * mu;
	float gg = gMie * gMie;
	float pRlh = 3.0 / (16.0 * M_PI) * (1.0 + mumu);
	float pMie = 3.0 / (8.0 * M_PI) * ((1.0 - gg) * (mumu + 1.0)) / (pow(1.0 + gg - 2.0 * mu * gMie, 1.5) * (2.0 + gg));

	float iTime = tA;
	float idS = (tB - tA) * inviSteps;

	float iOdRlh = 0.0;
	float iOdMie = 0.0;

	// Initialize accumulators for Rayleigh and Mie scattering.
	float3 totalRlh = float3(0, 0, 0);
	float3 totalMie = float3(0, 0, 0);

	for (int i = 0; i < iSteps; i++)
	{
		float3 iPos = rayOrigin + rayDir * (iTime + idS * 0.5);

		float iHeight = length(iPos) - rPlanet;

		// Calculate the optical depth of the Rayleigh and Mie scattering for this step.
		float odStepRlh = exp(-iHeight * invshRlh) * idS;
		float odStepMie = exp(-iHeight * invshMie) * idS;

		// Accumulate optical depth.
		iOdRlh += odStepRlh;
		iOdMie += odStepMie;

		// Initialize optical depth accumulators for the secondary ray.
		float jOdRlh = 0.0;
		float jOdMie = 0.0;

		float jdS = rsi(iPos, sunPos, rAtmos).y * invjSteps;

		// Initialize the secondary ray time.
		float jTime = 0.0;

		// Sample the secondary ray.
		for (int j = 0; j < jSteps; j++) {

			// Calculate the secondary ray sample position.
			float3 jPos = iPos + sunPos * (jTime + jdS * 0.5);

			// Calculate the height of the sample.
			float jHeight = length(jPos) - rPlanet;

			// Accumulate the optical depth.
			jOdRlh += exp(-jHeight * invshRlh) * jdS;
			jOdMie += exp(-jHeight * invshMie) * jdS;

			// Increment the secondary ray time.
			jTime += jdS;
		}

		// Calculate attenuation.
		float3 attn = exp(-(kMie * (iOdMie + jOdMie) + kRlh * (iOdRlh + jOdRlh)));

		// Accumulate scattering.
		totalRlh += odStepRlh * attn;
		totalMie += odStepMie * attn;

		iTime += idS;
	}

	return sunIntensity * (pRlh * kRlh * totalRlh + pMie * kMie * totalMie);
}