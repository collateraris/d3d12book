#include "CommonInclude.hlsl"

struct GRID_PARAMS
{
	matrix gridViewProjMatrices;
	float4 gridCellSizes;
	float4 gridPositions;
	float4 snappedGridPositions;
	float4 lastSnappedGridPositions;
	float4 globalIllumParams;
};

ConstantBuffer<GRID_PARAMS> bGridParams : register(b1);

struct VOXEL
{
	uint4 normalMasks; // encoded normals
	uint colorMask; // encoded color
	uint occlusion; // voxel only contains geometry info if occlusion > 0
};

StructuredBuffer<VOXEL> tVoxelsGrid : register(t0);

struct VertexShaderOutput
{
	float4 Position    : SV_POSITION;
	float3 PositionWS  : POS_WS;
	float3 Normal      : NORMAL;
	float2 TexCoord    : TEXCOORD;
};

struct FragmentShaderOutput
{
	float4 fragColor: SV_TARGET;
};

// get index into a 32x32x32 grid for the specified position
int GetGridIndex(in int3 position)
{
	return ((position.z * bGridParams.gridCellSizes.z * bGridParams.gridCellSizes.z) + (position.y * bGridParams.gridCellSizes.z) + position.x);
}

FragmentShaderOutput main(VertexShaderOutput input)
{
	FragmentShaderOutput output;

	float3 color = float3(1.f, 0.f, 1.f);

	float3 offset = (input.PositionWS.xyz - bGridParams.snappedGridPositions.xyz) * bGridParams.gridCellSizes.y;
	offset = round(offset);

	// get position in the voxel-grid
	int centerVoxelGrid = int(bGridParams.gridCellSizes.z * 0.5);
	int3 voxelPos = int3(centerVoxelGrid, centerVoxelGrid, centerVoxelGrid) + int3(offset.x, offset.y, offset.z);

	// To avoid raise conditions between multiple threads, that write into the same location, atomic
	// functions have to be used. Only output voxels that are inside the boundaries of the grid.
	if ((voxelPos.x > -1) && (voxelPos.x < bGridParams.gridCellSizes.z)
		&& (voxelPos.y > -1) && (voxelPos.y < bGridParams.gridCellSizes.z)
		&& (voxelPos.z > -1) && (voxelPos.z < bGridParams.gridCellSizes.z))
	{
		int gridIndex = GetGridIndex(voxelPos);

		// get voxel
		VOXEL voxel = tVoxelsGrid[gridIndex];

		// decode color
		color = DecodeColor(voxel.colorMask);
	};

	output.fragColor = float4(color, 1.0f);

	return output;
}