struct PixelShaderInput
{
    float4 PositionVS : POSITION;
    float3 NormalVS   : NORMAL;
    float2 TexCoord   : TEXCOORD;
};

struct LightBuffer
{
    float4 ambientColor;
    float4 diffuseColor;
    float3 lightDirection;
};

ConstantBuffer<LightBuffer> dirLight : register(b1);

Texture2D AmbientTexture  : register(t0);

SamplerState LinearRepeatSampler : register(s0);


float4 main(PixelShaderInput IN) : SV_Target0
{
    float4 textureColor = AmbientTexture.Sample(LinearRepeatSampler, IN.TexCoord);
    float4 color = dirLight.ambientColor;

    // Invert the light direction for calculations.
    float3 lightDir = -dirLight.lightDirection;

    // Calculate the amount of light on this pixel.
    float lightIntensity = saturate(dot(IN.NormalVS, lightDir));

    if (lightIntensity > 0.0f)
    {
        // Determine the final diffuse color based on the diffuse color and the amount of light intensity.
        color += (dirLight.diffuseColor * lightIntensity);
    }

    // Saturate the final light color.
    color = saturate(color);

    // Multiply the texture pixel and the final light color to get the result.
    color = color * textureColor;
    //return float4(0.5, 0.5, 0.5, 1.f);
    return color;
}

