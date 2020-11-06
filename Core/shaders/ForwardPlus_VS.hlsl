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
    float2 TexCoord    : TEXCOORD;
    float4 Position    : SV_Position;
    float3 NormalVS    : NORMAL;
    float3 TangentVS   : TANGENT;      // View space tangent.
    float3 BinormalVS  : BINORMAL;     // View space binormal.
};

VertexShaderOutput main(VertexPositionNormalTexture IN)
{
    VertexShaderOutput OUT;

    OUT.PositionVS = mul(MatCB.ModelViewMatrix, float4(IN.Position, 1.0f));
    OUT.Position = mul(MatCB.ModelViewProjectionMatrix, float4(IN.Position, 1.0f));
    OUT.TexCoord = IN.TexCoord;
    OUT.NormalVS = mul((float3x3)MatCB.ModelViewMatrix, IN.Normal);
    OUT.TangentVS = mul((float3x3)MatCB.ModelViewMatrix, IN.Tangent);
    OUT.BinormalVS = mul((float3x3)MatCB.ModelViewMatrix, IN.Bitangent);

    return OUT;
}