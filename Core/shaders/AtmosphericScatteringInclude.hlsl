struct ComputeShaderInput
{
    uint3 groupID           : SV_GroupID;           // 3D index of the thread group in the dispatch.
    uint3 groupThreadID     : SV_GroupThreadID;     // 3D index of local thread ID in a thread group.
    uint3 dispatchThreadID  : SV_DispatchThreadID;  // 3D index of global thread ID in the dispatch.
    uint  groupIndex        : SV_GroupIndex;        // Flattened local index of the thread within a thread group.
};

struct AtmosphericScatteringParams
{
    float _AtmosphereHeight;
    float _PlanetRadius;
    float _MieG;
    float _DistanceScale;

    float4 _ScatteringR;
    float4 _ScatteringM;

    float4 _ExtinctionR;
    float4 _ExtinctionM;

    float2 _DensityScaleHeight;
    float _SunIntensity;
    float  padding;
};

struct ScatteringOutput
{
    float3 rayleigh;
    float3 mie;
    float2 padding;
};

float2 RaySphereIntersection(in float3 rayOrigin, in float3 rayDir, in float3 sphereCenter, float sphereRadius);

void ComputeLocalInscattering(in float2 localDensity, in float2 densityPA, in float2 densityCP, in float4 ExtinctionR, in float4 ExtinctionM, out float3 localInscatterR, out float3 localInscatterM);

float2 PrecomputeParticleDensity(float3 rayStart, float3 rayDir, float planetRadius, float atmosphereHeight, float2 densityScaleHeight);

float3 ApplyPhaseFunctionRiel(in float3 scatterR, float cosAngle);

float3 ApplyPhaseFunctionMie(in float3 scatterM, float cosAngle);

float Sun(float cosAngle);

float3 RenderSun(in float3 scatterM, float cosAngle);

// it`s fake. not work
[numthreads(1, 1, 1)]
void main_fake(ComputeShaderInput IN)
{

}

float2 RaySphereIntersection(in float3 rayOrigin, in float3 rayDir, float3 sphereCenter, float sphereRadius)
{
    rayOrigin -= sphereCenter;
    float a = dot(rayDir, rayDir);
    float b = 2.0 * dot(rayOrigin, rayDir);
    float c = dot(rayOrigin, rayOrigin) - (sphereRadius * sphereRadius);
    float d = b * b - 4 * a * c;
    if (d < 0)
    {
        return -1;
    }
    else
    {
        d = sqrt(d);
        return float2(-b - d, -b + d) / (2 * a);
    }
}

void ComputeLocalInscattering(in float2 localDensity, in float2 densityPA, in float2 densityCP, in float4 ExtinctionR, in float4 ExtinctionM, out float3 localInscatterR, out float3 localInscatterM)
{
    float2 densityCPA = densityCP + densityPA;

    float3 Tr = densityCPA.x * ExtinctionR.xyz;
    float3 Tm = densityCPA.y * ExtinctionM.xyz;

    float3 extinction = exp(-(Tr + Tm));

    localInscatterR = localDensity.x * extinction;
    localInscatterM = localDensity.y * extinction;
}

float2 PrecomputeParticleDensity(float3 rayOrigin, float3 rayDir, float planetRadius, float atmosphereHeight, float2 densityScaleHeight)
{
    float3 planetCenter = float3(0., -planetRadius, 0.);

    float stepNum = 250.;

    float2 intersection = RaySphereIntersection(rayOrigin, rayDir, planetCenter, planetRadius);

    if (intersection.x > 0)
        return 1e+20;

    intersection = RaySphereIntersection(rayOrigin, rayDir, planetCenter, planetRadius + atmosphereHeight);
    float3 rayEnd = rayOrigin + rayDir * intersection.y;

    float3 step = (rayEnd - rayOrigin) / stepNum;
    float stepSize = length(step);
    float2 density = 0;

    for (float s = 0.5; s < stepNum; s += 1.0)
    {
        float3 pos = rayOrigin + step * s;
        float height = abs(length(pos - planetCenter) - planetRadius);
        float2 localDensity = exp(-(height.xx / densityScaleHeight));

        density += localDensity * stepSize;
    }

    return density;
}

float3 ApplyPhaseFunctionRiel(in float3 scatterR, float cosAngle)
{
    const float PI = 3.14159265359;
    float phase = (3.0 / (16.0 * PI)) * (1 + (cosAngle * cosAngle));
    return scatterR * phase;
}

float3 ApplyPhaseFunctionMie(in float3 scatterM, float cosAngle, float mieG)
{
    const float PI = 3.14159265359;
    float g = mieG;
    float g2 = g * g;
    float phase = (1.0 / (4.0 * PI)) * ((3.0 * (1.0 - g2)) / (2.0 * (2.0 + g2))) * ((1 + cosAngle * cosAngle) / (pow((1 + g2 - 2 * g * cosAngle), 3.0 / 2.0)));
    return scatterM * phase;
}

float Sun(float cosAngle)
{
    const float PI = 3.14159265359;
    float g = 0.98;
    float g2 = g * g;

    float sun = pow(1 - g, 2.0) / (4 * PI * pow(1.0 + g2 - 2.0 * g * cosAngle, 1.5));
    return sun * 0.003;// 5;
}

float3 RenderSun(in float3 scatterM, float cosAngle)
{
    return scatterM * Sun(cosAngle);
}

