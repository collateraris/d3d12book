struct Mat
{
    matrix MVP;
};

struct VertexShaderInput
{
    float3 Position : POSITION;
    float3 TexCoord : TEXCOORD;
};

ConstantBuffer<Mat> MatCB : register(b0);

struct VertexShaderOutput
{
    float3 TexCoord : TEXCOORD;
    float4 Position : SV_POSITION;
};

VertexShaderOutput main(VertexShaderInput IN)
{
    VertexShaderOutput OUT;

    OUT.Position = mul(MatCB.MVP, float4(IN.Position, 1.0f));
    OUT.TexCoord = IN.TexCoord;

    return OUT;
}