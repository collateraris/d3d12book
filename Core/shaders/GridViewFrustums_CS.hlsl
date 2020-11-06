#include "CommonInclude.hlsl"

#define BLOCK_SIZE 16

// View space frustums for the grid cells.
RWStructuredBuffer<Frustum> uOutFrustum : register(u0);

// Parameters required to convert screen space coordinates to view space params.
ConstantBuffer<ScreenToViewParams> bScreenToViewParams : register(b0);
ConstantBuffer<DispatchParams> bDispatchParams : register(b1);

// A kernel to compute frustums for the grid
// This kernel is executed once per grid cell. Each thread
// computes a frustum for a grid cell.
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void main(ComputeShaderInput IN)
{
	// View space eye position is always at the origin.
	const float3 eyePos = float3(0., 0., 0.);

    const int PLANE_IN_FRUSTUM = 4;
	// Compute 4 points on the far clipping plane to use as the 
	// frustum vertices.
	float4 screenSpace[PLANE_IN_FRUSTUM];

    // Top left point
    screenSpace[0] = float4(IN.dispatchThreadID.xy * BLOCK_SIZE, -1.0f, 1.0f);
    // Top right point
    screenSpace[1] = float4(float2(IN.dispatchThreadID.x + 1, IN.dispatchThreadID.y) * BLOCK_SIZE, -1.0f, 1.0f);
    // Bottom left point
    screenSpace[2] = float4(float2(IN.dispatchThreadID.x, IN.dispatchThreadID.y + 1) * BLOCK_SIZE, -1.0f, 1.0f);
    // Bottom right point
    screenSpace[3] = float4(float2(IN.dispatchThreadID.x + 1, IN.dispatchThreadID.y + 1) * BLOCK_SIZE, -1.0f, 1.0f);

    float4 viewSpace[PLANE_IN_FRUSTUM];

    for (int i = 0; i < PLANE_IN_FRUSTUM; ++i)
    {
        ScreenToView(screenSpace[i], bScreenToViewParams.ScreenDimensions, bScreenToViewParams.InverseProjection, viewSpace[i]);
    }

    // Now build the frustum planes from the view space points
    Frustum frustum;
    //clockwise orientation
    // Left plane
    ComputePlane(eyePos, viewSpace[2], viewSpace[0], frustum.planes[0]);
    // Right plane
    ComputePlane(eyePos, viewSpace[1], viewSpace[3], frustum.planes[1]);
    // Top plane
    ComputePlane(eyePos, viewSpace[0], viewSpace[1], frustum.planes[2]);
    // Bottom plane
    ComputePlane(eyePos, viewSpace[3], viewSpace[2], frustum.planes[3]);

    // Store the computed frustum in global memory (if our thread ID is in bounds of the grid).
    float2 numThreads = bDispatchParams.numThreads;
    if (IN.dispatchThreadID.x < numThreads.x && IN.dispatchThreadID.y < numThreads.y)
    {
        uint index = IN.dispatchThreadID.x + (IN.dispatchThreadID.y * numThreads.x);
        uOutFrustum[index] = frustum;
    }
}