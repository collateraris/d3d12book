#pragma once

#include <URootObject.h>
#include <QuadTree.h>

#include <string>

namespace dx12demo::core
{
	struct TerrainInfo
	{
		float normalizeHeightMapCoef = 15.f;
		std::string heightMapPath = "";
		int textureRepeat = 8;
	};

	class CommandList;

	class Terrain : public URootObject
	{
	public:

		Terrain();
		virtual ~Terrain();

		void Generate(CommandList& commandList, const TerrainInfo&);

		void Render(CommandList& commandList);

	private:

		void LoadNormalizedHeightMap(const TerrainInfo&, std::vector<DirectX::XMFLOAT3>& heightMap, int& terrainWidth, int& terrainHeight);

		void CalculateNormals(std::vector<DirectX::XMFLOAT3>& normalMap, const std::vector<DirectX::XMFLOAT3>& heightMap, int terrainWidth, int terrainHeight);

		void CalculateTextureCoordinates(const TerrainInfo&, std::vector<DirectX::XMFLOAT2>& texCoordMap, int terrainWidth, int terrainHeight);

		std::unique_ptr<QuadTree<PosNormTexVertex>> m_terrainMesh;
	};
}