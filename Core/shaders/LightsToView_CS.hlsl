#include "CommonInclude.hlsl"

#define BLOCK_SIZE 1

struct ModelViewMatrix
{
    matrix _matrix;
};

// Parameters required to convert screen space coordinates to view space params.
ConstantBuffer<ModelViewMatrix> bModelView : register(b0);

struct LightInfo
{
    int NUM_LIGHTS;
};

StructuredBuffer<LightInfo> tLightInfo : register(t0);
RWStructuredBuffer<Light> uLights : register(u0);

[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void main(ComputeShaderInput IN)
{
    uint numLights = tLightInfo[0].NUM_LIGHTS;
    for (uint i = 0; i < numLights; ++i)
    {
        uLights[i].PositionVS = mul(bModelView._matrix, uLights[i].PositionWS);
        uLights[i].DirectionVS = mul(bModelView._matrix, uLights[i].DirectionWS);
    }
}