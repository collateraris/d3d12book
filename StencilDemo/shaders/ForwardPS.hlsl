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

Texture2D AmbientTexture  : register(t0);

SamplerState LinearAnisotropicWrap : register(s0);

float4 main(PixelShaderInput IN) : SV_Target0
{
    float4 textureColor = AmbientTexture.Sample(LinearAnisotropicWrap, IN.TexCoord);
    float4 color = dirLightCB.ambientColor;

    // Invert the light direction for calculations.
    float3 lightDir = -dirLightCB.lightDirection;

    // Calculate the amount of light on this pixel.
    float lightIntensity = saturate(dot(IN.NormalVS, lightDir));

    if (lightIntensity > 0.0f)
    {
        // Determine the final diffuse color based on the diffuse color and the amount of light intensity.
        color += (dirLightCB.strength * lightIntensity);
    }

    // Saturate the final light color.
    color = saturate(color);

    // Multiply the texture pixel and the final light color to get the result.
    color = color * textureColor;
    color.a = materialsCB.diffuseAlbedo.a;
    //return float4(0.5, 0.5, 0.5, 1.f);
    return color;
}

