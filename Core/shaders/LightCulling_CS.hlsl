#include "CommonInclude.hlsl"

#define BLOCK_SIZE 16

// Parameters required to convert screen space coordinates to view space params.
ConstantBuffer<ScreenToViewParams> bScreenToViewParams : register(b0);
ConstantBuffer<DispatchParams> bDispatchParams : register(b1);

struct LightInfo
{
    int NUM_LIGHTS;
};

// The depth from the screen space texture.
Texture2D tDepthTextureVS : register(t0);
// Precomputed frustums for the grid.
StructuredBuffer<Frustum> tFrustums : register(t1);

// Debug texture for debugging purposes.
Texture2D tLightCountHeatMap : register(t2);
StructuredBuffer<Light> Lights : register(t3);
StructuredBuffer<LightInfo> tLightInfo : register(t4);
RWTexture2D<float4> uDebugTexture : register(u0);
SamplerState sLinearClampSampler : register(s0);

// Global counter for current index into the light index list.
// "o_" prefix indicates light lists for opaque geometry while 
// "t_" prefix indicates light lists for transparent geometry.
globallycoherent RWStructuredBuffer<uint> o_uLightIndexCounter : register(u1);
globallycoherent RWStructuredBuffer<uint> t_uLightIndexCounter : register(u2);

// Light index lists and light grids.
RWStructuredBuffer<uint> o_uLightIndexList : register(u3);
RWStructuredBuffer<uint> t_uLightIndexList : register(u4);
RWTexture2D<uint2> o_uLightGrid : register(u5);
RWTexture2D<uint2> t_uLightGrid : register(u6);

// Group shared variables.
groupshared uint gsMinDepth;
groupshared uint gsMaxDepth;
groupshared Frustum gsGroupFrustum;

// Opaque geometry light lists.
groupshared uint o_gsLightCount;
groupshared uint o_gsLightIndexStartOffset;
groupshared uint o_gsLightList[1024];

// Transparent geometry light lists.
groupshared uint t_gsLightCount;
groupshared uint t_gsLightIndexStartOffset;
groupshared uint t_gsLightList[1024];

// Add the light to the visible light list for opaque geometry.
void o_AppendLight(uint lightIndex)
{
    uint index; // Index into the visible lights array.
    InterlockedAdd(o_gsLightCount, 1, index);
    if (index < 1024)
    {
        o_gsLightList[index] = lightIndex;
    }
}

// Add the light to the visible light list for transparent geometry.
void t_AppendLight(uint lightIndex)
{
    uint index; // Index into the visible lights array.
    InterlockedAdd(t_gsLightCount, 1, index);
    if (index < 1024)
    {
        t_gsLightList[index] = lightIndex;
    }
}

[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void main(ComputeShaderInput IN)
{
    // Calculate min & max depth in threadgroup / tile.
    int2 texCoord = IN.dispatchThreadID.xy;
    float f_Depth = tDepthTextureVS.Load(int3(texCoord, 0)).r;

    uint u_Depth = f_Depth;

    if (IN.groupIndex == 0) // Avoid contention by other threads in the group.
    {
        gsMinDepth = 0xffffffff;
        gsMaxDepth = 0;
        o_gsLightCount = 0;
        t_gsLightCount = 0;
        gsGroupFrustum = tFrustums[IN.groupID.x + (IN.groupID.y * bDispatchParams.numThreadGroups.x)];
    }

    GroupMemoryBarrierWithGroupSync();

    InterlockedMin(gsMinDepth, u_Depth);
    InterlockedMax(gsMaxDepth, u_Depth);

    GroupMemoryBarrierWithGroupSync();

    float f_MinDepth = float(gsMinDepth);
    float f_MaxDepth = float(gsMaxDepth);

    float4 view_tmp; 
    // Convert depth values to view space.
    ScreenToView(float4(0, 0, f_MinDepth, 1), bScreenToViewParams.ScreenDimensions, bScreenToViewParams.InverseProjection, view_tmp);
    float minDepthVS = view_tmp.z;
    ScreenToView(float4(0, 0, f_MaxDepth, 1), bScreenToViewParams.ScreenDimensions, bScreenToViewParams.InverseProjection, view_tmp);
    float maxDepthVS = view_tmp.z;
    ScreenToView(float4(0, 0, 0, 1), bScreenToViewParams.ScreenDimensions, bScreenToViewParams.InverseProjection, view_tmp);
    float nearClipVS = view_tmp.z;

    // Clipping plane for minimum depth value 
    // (used for testing lights within the bounds of opaque geometry).
    Plane minPlane = { 0, 0, -1, -minDepthVS };

    // Cull lights
    // Each thread in a group will cull 1 light until all lights have been culled.
    uint sqBLOCK_SIZE = BLOCK_SIZE * BLOCK_SIZE;
    uint numLights = tLightInfo[0].NUM_LIGHTS;
    for (uint i = IN.groupIndex; i < numLights; i += sqBLOCK_SIZE)
    {
        if (Lights[i].Enabled)
        {
            Light light = Lights[i];
            switch (light.Type)
            {
            case POINT_LIGHT:
            {
                Sphere sphere = { light.PositionVS.xyz, light.Range };
                if (SphereInsideFrustum(sphere, gsGroupFrustum, nearClipVS, maxDepthVS))
                {
                    // Add light to light list for transparent geometry.
                    t_AppendLight(i);

                    if (!SphereInsidePlane(sphere, minPlane))
                    {
                        // Add light to light list for opaque geometry.
                        o_AppendLight(i);
                    }
                }
            }
            break;
            case SPOT_LIGHT:
            {
                float coneRadius = tan(radians(light.SpotAngle)) * light.Range;
                Cone cone = { light.PositionVS.xyz, light.Range, light.DirectionVS.xyz, coneRadius };
                if (ConeInsideFrustum(cone, gsGroupFrustum, nearClipVS, maxDepthVS))
                {
                    // Add light to light list for transparent geometry.
                    t_AppendLight(i);

                    if (!ConeInsidePlane(cone, minPlane))
                    {
                        // Add light to light list for opaque geometry.
                        o_AppendLight(i);
                    }
                }
            }
            break;
            case DIRECTIONAL_LIGHT:
            {
                // Directional lights always get added to our light list.
                // (Hopefully there are not too many directional lights!)
                t_AppendLight(i);
                o_AppendLight(i);
            }
            break;
            }
        }
    }

    GroupMemoryBarrierWithGroupSync();

    if (IN.groupIndex == 0)
    {
        InterlockedAdd(o_uLightIndexCounter[0], o_gsLightCount, o_gsLightIndexStartOffset);
        o_uLightGrid[IN.groupID.xy] = uint2(o_gsLightIndexStartOffset, o_gsLightCount);

        InterlockedAdd(t_uLightIndexCounter[0], t_gsLightCount, t_gsLightIndexStartOffset);
        t_uLightGrid[IN.groupID.xy] = uint2(t_gsLightIndexStartOffset, t_gsLightCount);
    }

    GroupMemoryBarrierWithGroupSync();

    for (uint i = IN.groupIndex; i < o_gsLightCount; i += sqBLOCK_SIZE)
    {
        o_uLightIndexList[o_gsLightIndexStartOffset + i] = o_gsLightList[i];
    }

    for (uint i = IN.groupIndex; i < t_gsLightCount; i += sqBLOCK_SIZE)
    {
        t_uLightIndexList[t_gsLightIndexStartOffset + i] = t_gsLightList[i];
    }

    if (IN.groupThreadID.x == 0 || IN.groupThreadID.y == 0)
    {
        uDebugTexture[texCoord] = float4(0, 0, 0, 0.9f);
    }
    else if (IN.groupThreadID.x == 1 || IN.groupThreadID.y == 1)
    {
        uDebugTexture[texCoord] = float4(1, 1, 1, 0.5f);
    }
    else if (o_gsLightCount > 0)
    {
        float f_numLights = float(numLights);
        float normalizedLightCount = o_gsLightCount / f_numLights;
        float4 lightCountHeatMapColor = tLightCountHeatMap.SampleLevel(sLinearClampSampler, float2(normalizedLightCount, 0), 0);
        uDebugTexture[texCoord] = lightCountHeatMapColor;
    }
    else
    {
        uDebugTexture[texCoord] = float4(0, 0, 0, 1);
    }
}