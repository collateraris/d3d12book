struct Mat
{
    matrix ModelMatrix;
    matrix ModelViewMatrix;
    matrix InverseTransposeModelViewMatrix;
    matrix ModelViewProjectionMatrix;
};

ConstantBuffer<Mat> MatCB : register(b0);

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
    float4 PositionVS  : POSITION;
    float3 NormalVS    : NORMAL;
    float2 TexCoord    : TEXCOORD;
    float3 TangentVS   : TANGENT;
    float3 BitangentVS : BITANGENT;
    float4 Position    : SV_Position;
};

VertexShaderOutput main(VertexPositionNormalTexture IN)
{
    VertexShaderOutput OUT;

    OUT.Position = mul(MatCB.ModelViewProjectionMatrix, float4(IN.Position, 1.0f));
    OUT.PositionVS = mul(MatCB.ModelViewMatrix, float4(IN.Position, 1.0f));
    OUT.NormalVS = mul((float3x3)MatCB.InverseTransposeModelViewMatrix, IN.Normal);
    OUT.TexCoord = IN.TexCoord;
    OUT.TangentVS = mul((float3x3)MatCB.ModelViewMatrix, IN.Tangent);
    OUT.BitangentVS = mul((float3x3)MatCB.ModelViewMatrix, IN.Bitangent);

    return OUT;
}