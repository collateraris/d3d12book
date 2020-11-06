
struct VertexShaderInput
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 TexC     : TEXCOORD;
};

struct VertexShaderOutput
{
    float4 Position : SV_POSITION;
    float2 TexC     : TEXCOORD;
};

VertexShaderOutput main(VertexShaderInput IN)
{
    VertexShaderOutput OUT;

    OUT.Position = float4(IN.Position, 1.0f);
    OUT.TexC = IN.TexC;

    return OUT;
}

