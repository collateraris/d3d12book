#pragma once

#include <URootObject.h>
#include <CommandList.h>
#include <RootSignature.h>
#include <StructuredBuffer.h>
#include <Texture.h>
#include <Camera.h>
#include <DirectXMath.h>

namespace dx12demo::core
{
	struct VoxelGridParams
	{
		DirectX::XMMATRIX gridViewProjMatrices[3];
		DirectX::XMFLOAT4 gridCellSizes;
		DirectX::XMVECTOR gridPositions;
		DirectX::XMVECTOR snappedGridPositions;
		DirectX::XMVECTOR lastSnappedGridPositions;
		DirectX::XMVECTOR globalIllumParams;
	};

	struct VoxelOfVoxelGridClass
	{
		DirectX::XMFLOAT4 color;
		uint32_t normalMasks[4]; // encoded normals
		uint32_t colorMask; // encoded color
		uint32_t occlusion; // voxel only contains geometry info if occlusion > 0
	};

	class VoxelGrid : URootObject
	{

	public:
		VoxelGrid();
		virtual ~VoxelGrid();

		void InitVoxelGrid(std::shared_ptr<CommandList>& commandList);
		void UpdateGrid(std::shared_ptr<CommandList>& commandList, Camera& camera);

		void AttachAmbientTex(std::shared_ptr<CommandList>& commandList, const Texture& tex);

		const VoxelGridParams& GetGridParams() const;
		const StructuredBuffer& GetVoxelGrid() const;

	private:

		RootSignature m_RootSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;

		VoxelGridParams m_GridParams;
		DirectX::XMMATRIX m_GridProjMatrix;
		const float m_GridHalfExtent = 1000.0f;

		StructuredBuffer m_VoxelGrid;
	};
}