struct Mat
{
    matrix ModelMatrix;
    matrix ModelViewMatrix;
    matrix InverseTransposeModelViewMatrix;
    matrix ModelViewProjectionMatrix;
};

ConstantBuffer<Mat> MatCB : register(b0);

struct VertexShaderInput
{
    float3 Position : POSITION;
    float3 Normal    : NORMAL;
    float2 TexCoord  : TEXCOORD;
};

struct VertexShaderOutput
{
    float4 Position : SV_POSITION;
};

VertexShaderOutput main(VertexShaderInput IN)
{
    VertexShaderOutput OUT;

    OUT.Position = mul(MatCB.ModelViewProjectionMatrix, float4(IN.Position, 1.0f));

    return OUT;
}

