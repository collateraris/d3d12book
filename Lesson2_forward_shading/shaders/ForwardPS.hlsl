struct PixelShaderInput
{
    float4 PositionVS : POSITION;
    float3 NormalVS   : NORMAL;
    float2 TexCoord   : TEXCOORD;
    float3 TangentVS   : TANGENT;
    float3 BitangentVS : BITANGENT;
};

struct PointLight
{
    float4 PositionWS; // Light position in world space.
    //----------------------------------- (16 byte boundary)
    float4 PositionVS; // Light position in view space.
    //----------------------------------- (16 byte boundary)
    float4 Color;
    //----------------------------------- (16 byte boundary)
    float       Intensity;
    float       Attenuation;
    float2      Padding;                // Pad to 16 bytes
    //----------------------------------- (16 byte boundary)
    // Total:                              16 * 4 = 64 bytes
};

struct SpotLight
{
    float4 PositionWS; // Light position in world space.
    //----------------------------------- (16 byte boundary)
    float4 PositionVS; // Light position in view space.
    //----------------------------------- (16 byte boundary)
    float4 DirectionWS; // Light direction in world space.
    //----------------------------------- (16 byte boundary)
    float4 DirectionVS; // Light direction in view space.
    //----------------------------------- (16 byte boundary)
    float4 Color;
    //----------------------------------- (16 byte boundary)
    float       Intensity;
    float       SpotAngle;
    float       Attenuation;
    float       Padding;                // Pad to 16 bytes.
    //----------------------------------- (16 byte boundary)
    // Total:                              16 * 6 = 96 bytes
};

struct LightProperties
{
    uint NumPointLights;
    uint NumSpotLights;
};

struct LightResult
{
    float4 Diffuse;
    float4 Specular;
};

ConstantBuffer<LightProperties> LightPropertiesCB : register(b1);

StructuredBuffer<PointLight> PointLights : register(t0);
StructuredBuffer<SpotLight> SpotLights : register(t1);

Texture2D AmbientTexture  : register(t2);
Texture2D DiffuseTexture  : register(t3);
Texture2D SpecularTexture : register(t4);
Texture2D NormalTexture   : register(t5);

SamplerState LinearRepeatSampler : register(s0);

void CollectMatColor(inout float4 matColor, in Texture2D matTex, in float2 texCoord);

float4 DoNormalMapping(in float3x3 TBN, in Texture2D tex, in sampler s, in float2 uv);

float3 ExpandNormal(in float3 n);

float DoAttenuation(float attenuation, float distance);

float DoDiffuse(in float3 N, in float3 L);

float DoSpecular(in float3 V, in float3 N, in float3 L);

LightResult DoPointLight(in PointLight light, in float3 V, in float3 P, in float3 N);

float DoSpotCone(in float3 spotDir, in float3 L, in float spotAngle);

LightResult DoSpotLight(in SpotLight light, in float3 V, in float3 P, in float3 N);

LightResult DoLighting(in float3 P, in float3 N);

float4 main(PixelShaderInput IN) : SV_Target0
{
   float4 ambient = float4(1.0f, 1.0f, 1.0f, 1.0f);
   CollectMatColor(ambient, AmbientTexture, IN.TexCoord);

   float4 diffuse = float4(1.0f, 1.0f, 1.0f, 1.0f);
   CollectMatColor(diffuse, DiffuseTexture, IN.TexCoord);

   float4 normal;
   float3x3 TBN = float3x3(normalize(IN.TangentVS),
       normalize(IN.BitangentVS),
       normalize(IN.NormalVS));
   normal = DoNormalMapping(TBN, NormalTexture, LinearRepeatSampler, IN.TexCoord);

   float4 specular = float4(1.0f, 1.0f, 1.0f, 1.0f);
   CollectMatColor(specular, SpecularTexture, IN.TexCoord);

   LightResult lightRes = DoLighting(IN.PositionVS.xyz, normal);

   diffuse *= lightRes.Diffuse;
   specular *= lightRes.Specular;

   float4 texColor = DiffuseTexture.Sample(LinearRepeatSampler, IN.TexCoord);

   return (ambient + diffuse + specular) * texColor;
}

void CollectMatColor(inout float4 matColor, in Texture2D matTex, in float2 texCoord)
{
    float4 texColor = matTex.Sample(LinearRepeatSampler, texCoord);
    if (any(matColor.rgb))
    {
        matColor *= texColor;
    }
    else
    {
        matColor = texColor;
    }
}

float4 DoNormalMapping(in float3x3 TBN, in Texture2D tex, in sampler s, in float2 uv)
{
    float3 normal = tex.Sample(s, uv).xyz;
    normal = ExpandNormal(normal);

    // Transform normal from tangent space to view space.
    normal = mul(normal, TBN);
    return normalize(float4(normal, 0));
}

float3 ExpandNormal(in float3 n)
{
    return n * 2.0f - 1.0f;
}

float DoAttenuation(float attenuation, float distance)
{
    return 1.0f / (1.0f + attenuation * distance * distance);
}

float DoDiffuse(in float3 N, in float3 L)
{
    return max(0, dot(N, L));
}

float DoSpecular(in float3 V, in float3 N, in float3 L)
{
    float3 R = normalize(reflect(-L, N));
    float RdotV = max(0, dot(R, V));

    const float specularPower = 16;
    return pow(RdotV, specularPower);
}

LightResult DoPointLight(in PointLight light, in float3 V, in float3 P, in float3 N)
{
    LightResult result;
    float3 L = (light.PositionVS.xyz - P);
    float d = length(L);
    L = L / d;

    float attenuation = DoAttenuation(light.Attenuation, d);

    result.Diffuse = DoDiffuse(N, L) * attenuation * light.Color * light.Intensity;
    result.Specular = DoSpecular(V, N, L) * attenuation * light.Color * light.Intensity;

    return result;
}

float DoSpotCone(in float3 spotDir, in float3 L, in float spotAngle)
{
    float minCos = cos(spotAngle);
    float maxCos = (minCos + 1.0f) / 2.0f;

    float cosAngle = dot(spotDir, -L);
    return smoothstep(minCos, maxCos, cosAngle);
}

LightResult DoSpotLight(in SpotLight light, in float3 V, in float3 P, in float3 N)
{
    LightResult result;
    float3 L = (light.PositionVS.xyz - P);
    float d = length(L);
    L = L / d;

    float attenuation = DoAttenuation(light.Attenuation, d);
    float spotIntensity = DoSpotCone(light.DirectionVS.xyz, L, light.SpotAngle);

    result.Diffuse = DoDiffuse(N, L) * attenuation * spotIntensity * light.Color * light.Intensity;
    result.Specular = DoSpecular(V, N, L) * attenuation * spotIntensity * light.Color * light.Intensity;

    return result;
}

LightResult DoLighting(in float3 P, in float3 N)
{
    uint i;

    // Lighting is performed in view space.
    float3 V = normalize(-P);

    LightResult totalResult = (LightResult)0;

    for (i = 0; i < LightPropertiesCB.NumPointLights; ++i)
    {
        LightResult result = DoPointLight(PointLights[i], V, P, N);

        totalResult.Diffuse += result.Diffuse;
        totalResult.Specular += result.Specular;
    }

    for (i = 0; i < LightPropertiesCB.NumSpotLights; ++i)
    {
        LightResult result = DoSpotLight(SpotLights[i], V, P, N);

        totalResult.Diffuse += result.Diffuse;
        totalResult.Specular += result.Specular;
    }

    return totalResult;
}