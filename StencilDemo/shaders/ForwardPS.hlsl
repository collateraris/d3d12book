struct PixelShaderInput
{
    float4 PositionVS : POSITION;
    float3 NormalVS   : NORMAL;
    float2 TexCoord   : TEXCOORD;
};

struct DirLight
{
    float4 ambientColor;
    float4 strength;
    float3 lightDirection;
};

ConstantBuffer<DirLight> dirLightCB : register(b1);

struct MaterialConstant
{
    float4 diffuseAlbedo;
    float3 freshnelR0;
    float Roughness;
};

ConstantBuffer<MaterialConstant> materialsCB : register(b2);

struct RenderPassData
{
    float4 AmbientLight;
    float3 ViewPos;
};

ConstantBuffer<RenderPassData> passDataCB : register(b3);

Texture2D AmbientTexture  : register(t0);

SamplerState LinearAnisotropicWrap : register(s0);

// Schlick gives an approximation to Fresnel reflectance
// R0 = ( (n-1)/(n+1) )^2, where n is the index of refraction.
void SchlickFresnel(in float3 R0, in float3 normal, in float3 lightVec, out float3 reflectPercent);

void BlinnPhong(in float3 lightStrength, in float3 lightVec, in float3 normal, float3 toEye, out float4 color);

void ComputeDirectionalLight(in float3 normal, in float3 toEye, out float4 color);

float4 main(PixelShaderInput IN) : SV_Target0
{
    float4 textureColor = AmbientTexture.Sample(LinearAnisotropicWrap, IN.TexCoord);
    float4 diffuseAlbedo = textureColor * materialsCB.diffuseAlbedo;
    float3 normal = normalize(IN.NormalVS);
    
    float3 posW = IN.PositionVS.xyz;
    float3 toEye = normalize(passDataCB.ViewPos - posW);
    float4 directLight;
    ComputeDirectionalLight(normal, toEye, directLight);

    // Saturate the final light color.
    float4 ambient = diffuseAlbedo * passDataCB.AmbientLight;
    float4 color = directLight + ambient;
    // Multiply the texture pixel and the final light color to get the result.
    color.a = materialsCB.diffuseAlbedo.a;
    //return float4(0.5f, 0.5f, 0.5f, 1.f);
    return color;
}

void SchlickFresnel(in float3 R0, in float3 normal, in float3 lightVec, out float3 reflectPercent)
{
    float cosIncidentAngle = saturate(dot(normal, lightVec));

    float f0 = 1.0f - cosIncidentAngle;
    reflectPercent = R0 + (1.0f - R0) * (f0 * f0 * f0 * f0 * f0);
}

void BlinnPhong(in float3 lightStrength, in float3 lightVec, in float3 normal, float3 toEye, out float4 color)
{
    const float m = (1.0f - materialsCB.Roughness) * 256.0f;
    float3 halfVec = normalize(toEye + lightVec);

    float roughnessFactor = (m + 8.0f) * pow(max(dot(halfVec, normal), 0.0f), m) * 0.125f;
    float3 fresnelFactor;
    SchlickFresnel(materialsCB.freshnelR0, halfVec, lightVec, fresnelFactor);

    float3 specAlbedo = fresnelFactor * roughnessFactor;

    // Our spec formula goes outside [0,1] range, but we are 
    // doing LDR rendering.  So scale it down a bit.
    specAlbedo = specAlbedo / (specAlbedo + 1.0f);

    color = float4((materialsCB.diffuseAlbedo.rgb + specAlbedo) * lightStrength, 1.0);
}

void ComputeDirectionalLight(in float3 normal, in float3 toEye, out float4 color)
{
    // The light vector aims opposite the direction the light rays travel.
    float3 lightVec = -dirLightCB.lightDirection;
    float3 lightIntensity = saturate(dot(lightVec, normal));
    float3 lightStrength = dirLightCB.strength * lightIntensity;

    BlinnPhong(lightStrength, lightVec, normal, toEye, color);
}

