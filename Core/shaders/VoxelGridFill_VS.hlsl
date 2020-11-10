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

struct Mat
{
    matrix ModelMatrix;
};

ConstantBuffer<Mat> bMatCB : register(b0);

VertexShaderOutput main(VertexPositionNormalTexture IN)
{
    VertexShaderOutput OUT;

    OUT.Position = mul(bMatCB.ModelMatrix, float4(IN.Position, 1.0f));
    OUT.TexCoord = IN.TexCoord;
    OUT.Normal  = IN.Normal;

    return OUT;
}