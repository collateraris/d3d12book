struct VertexPositionNormalTexture
{
    float3 Position  : POSITION;
    float3 Normal    : NORMAL;
    float2 TexCoord  : TEXCOORD;
    float3 Tangent   : TANGENT;
    float3 Bitangent : BITANGENT;
};

struct VertexShaderOutput
{
    float4 Position    : POSITION;
    float3 Normal      : NORMAL;
    float2 TexCoord    : TEXCOORD;
};

VertexShaderOutput main(VertexPositionNormalTexture IN)
{
    VertexShaderOutput OUT;

    OUT.Position = float4(IN.Position, 1.0f);
    OUT.TexCoord = IN.TexCoord;
    OUT.Normal  = IN.Normal;

    return OUT;
}