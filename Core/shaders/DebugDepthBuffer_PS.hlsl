
Texture2D<float3> DebugDepthTexture : register(t0);
SamplerState LinearWrapSampler : register(s0);

struct VertexShaderOutput
{
    float4 Position : SV_POSITION;
    float2 TexC    : TEXCOORD;
};

float LinearizeDepth(in float depth)
{
    float zNear = 0.1;
    float zFar = 1000.0;
    float z = depth * 2.0 - 1.0;
    return (2.0 * zNear * zFar) / (zFar + zNear - z * (zFar - zNear)) / zFar;
}

float4 main(VertexShaderOutput IN) : SV_Target
{
    float depth = DebugDepthTexture.Sample(LinearWrapSampler, IN.TexC).r;
    depth = LinearizeDepth(depth);
    return float4(depth, depth, depth, 1.0f);
}