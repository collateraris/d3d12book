#include "CommonInclude.hlsl"

struct GRID_PARAMS
{
	matrix gridViewProjMatrices[3];
	float4 gridCellSizes;
	float4 gridPositions;
	float4 snappedGridPositions;
	float4 lastSnappedGridPositions;
	float4 globalIllumParams;
};

ConstantBuffer<GRID_PARAMS> bGridParams : register(b0);

struct Mat
{
	matrix InverseViewProjectionMatrix;
};

ConstantBuffer<Mat> bMatCB : register(b1);

Texture2D tDepthTex: register(t0);

struct VOXEL
{
	float4 color;
	uint4 normalMasks; // encoded normals
	uint colorMask; // encoded color
	uint occlusion; // voxel only contains geometry info if occlusion > 0
};

StructuredBuffer<VOXEL> tVoxelsGrid : register(t1);

struct VertexShaderOutput
{
	float2 TexCoord : TEXCOORD;
	float4 Position : SV_Position;
};

struct FragmentShaderOutput
{
	float4 fragColor: SV_TARGET;
};

// get index into a 32x32x32 grid for the specified position
int GetGridIndex(in int3 position)
{
	return ((position.z * 1024) + (position.y * 32) + position.x);
}

FragmentShaderOutput main(VertexShaderOutput input)
{
	FragmentShaderOutput output;
	float depth = tDepthTex.Load(int3(input.Position.xy, 0)).x;

	// reconstruct world-space position from depth
	float4 projPosition = float4(input.TexCoord, depth, 1.0f);
	projPosition.xy = (projPosition.xy * 2.0f) - 1.0f;
	projPosition.y = -projPosition.y;
	float4 position = mul(bMatCB.InverseViewProjectionMatrix, projPosition);
	position.xyz /= position.w;

	float3 color = float3(0.5f, 0.5f, 0.5f);

	float3 offset = (position.xyz - bGridParams.snappedGridPositions.xyz) * bGridParams.gridCellSizes.y;
	float squaredDist = dot(offset, offset);
	if (squaredDist <= (15.0f * 15.0f))
	{
		// get index of current voxel
		offset = round(offset);
		int3 voxelPos = int3(16, 16, 16) + int3(offset.x, offset.y, offset.z);
		int gridIndex = GetGridIndex(voxelPos);

		// get voxel
		VOXEL voxel = tVoxelsGrid[gridIndex];

		// decode color
		//color = DecodeColor(voxel.colorMask);
		color = voxel.color.rgb;
	};

	output.fragColor = float4(color, 1.0f);

	return output;
}