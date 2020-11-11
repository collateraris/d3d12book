struct VertexShaderOutput
{
    float2 TexCoord : TEXCOORD;
    float4 Position : SV_Position;
};

static const float4 sc_positionTexCoords[3] =
{
      float4(-1.0f, -1.0f, 0.0f, 1.0f),
      float4(3.0f, -1.0f, 2.0f, 1.0f),
      float4(-1.0f, 3.0f, 0.0f, -1.0f)
};

VertexShaderOutput main(uint VertexID : SV_VertexID)
{
    VertexShaderOutput output;
    float4 outputPositionTexCoords = sc_positionTexCoords[VertexID];
    output.Position = float4(outputPositionTexCoords.xy, 0.0f, 1.0f);
    output.TexCoord = outputPositionTexCoords.zw;
    return output;
}