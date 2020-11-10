#pragma once

#include <URootObject.h>
#include <CommandList.h>
#include <RootSignature.h>
#include <StructuredBuffer.h>
#include <Texture.h>
#include <Camera.h>
#include <RenderTarget.h>
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
		uint32_t normalMasks[4]; // encoded normals
		uint32_t colorMask; // encoded color
		uint32_t occlusion; // voxel only contains geometry info if occlusion > 0
	};

	class VoxelGrid : URootObject
	{
		struct Mat
		{
			DirectX::XMMATRIX model;
		};
	public:
		VoxelGrid(int gridSize, float gridHalfExtent = 1000.0f, int renderTargetSize = 64);
		VoxelGrid();
		virtual ~VoxelGrid();

		void InitVoxelGrid(std::shared_ptr<CommandList>& commandList);
		void UpdateGrid(std::shared_ptr<CommandList>& commandList, Camera& camera);

		void AttachAmbientTex(std::shared_ptr<CommandList>& commandList, const Texture& tex);
		void AttachModelMatrix(std::shared_ptr<CommandList>& commandList, const DirectX::XMMATRIX& model);

		const VoxelGridParams& GetGridParams() const;
		const StructuredBuffer& GetVoxelGrid() const;

	private:

		void Init();

		RootSignature m_RootSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;

		VoxelGridParams m_GridParams;
		DirectX::XMMATRIX m_GridProjMatrix;
		const float m_GridHalfExtent = 1000.0f;

		RenderTarget m_RenderTarget;
		D3D12_RECT m_ScissorRect;

		Mat m_Mat;

		StructuredBuffer m_VoxelGrid;

		int m_GridNum = 32;
		int m_RenderTargetSize = 64;
	};
}