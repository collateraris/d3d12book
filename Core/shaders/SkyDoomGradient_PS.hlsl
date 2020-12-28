struct PixelShaderInput
{
    // Skybox texture coordinate
    float3 domePosition : TEXCOORD;
};

static const float4 apexColor = float4(0.0f, 0.15f, 0.66f, 1.0f);
static const float4 centerColor = float4(0.81f, 0.38f, 0.66f, 1.0f);

float4 main(PixelShaderInput IN) : SV_Target
{
    // Determine the position on the sky dome where this pixel is located.
    float height = IN.domePosition.y;

    // The value ranges from -1.0f to +1.0f so change it to only positive values.
    if (height < 0.0)
    {
        height = 0.0f;
    }

    // Determine the gradient color by interpolating between the apex and center based on the height of the pixel in the sky dome.
    return lerp(centerColor, apexColor, height);
}