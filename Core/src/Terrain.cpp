#include <Terrain.h>

#include <DX12LibPCH.h>
#include <CommandList.h>
#include <Frustum.h>

using namespace dx12demo::core;

Terrain::Terrain()
{

}

Terrain::~Terrain()
{

}

void Terrain::Generate(CommandList& commandList, const TerrainInfo& info)
{
	assert(!info.heightMapPath.empty());

	if (info.heightMapPath.empty())
		return;

	// Calculate the number of vertices in the terrain mesh.
	int terrainWidth, terrainHeight;
	std::vector<DirectX::XMFLOAT3> heightMap;
	LoadNormalizedHeightMap(info, heightMap, terrainWidth, terrainHeight);
	std::vector<DirectX::XMFLOAT3> normalMap;
	CalculateNormals(normalMap, heightMap, terrainWidth, terrainHeight);
	std::vector<DirectX::XMFLOAT2> texCoordsMap;
	CalculateTextureCoordinates(info, texCoordsMap, terrainWidth, terrainHeight);

	int terrainWidthBorder = terrainWidth - 1;
	int terrainHeightBorder = terrainHeight - 1;
	int verticesPerQuad = 6;
	int vertexCount = terrainWidthBorder * terrainHeightBorder * verticesPerQuad;

	VertexCollection vertices;
	vertices.reserve(vertexCount);

	int bottomLeftIndex, bottomRightIndex,
		upperLeftIndex, upperRightIndex;

	float tv, tu;

	int currIndex = 0;

	for (int j = 0; j < terrainHeightBorder; ++j)
	{
		for (int i = 0; i < terrainWidthBorder; ++i)
		{
			bottomLeftIndex = terrainHeight * j + i;
			bottomRightIndex = terrainHeight * j + i + 1;
			upperRightIndex = terrainHeight * (j + 1) + i + 1;
			upperLeftIndex = terrainHeight * (j + 1) + i;

			// Upper left.
			tv  = texCoordsMap[upperLeftIndex].y;

			// Modify the texture coordinates to cover the top edge.
			if (tv == 1.0f) { tv = 0.0f; }

			{
				auto& upperLeftPos = heightMap[upperLeftIndex];
				auto& upperLeftNorm = normalMap[upperLeftIndex];
				auto& upperLeftUV = texCoordsMap[upperLeftIndex];
				vertices.emplace_back(PosNormTexVertex(upperLeftPos, upperLeftNorm, { upperLeftUV.x, tv }));
			}

			// Upper right.
			tu = texCoordsMap[upperRightIndex].x;
			tv = texCoordsMap[upperRightIndex].y;

			// Modify the texture coordinates to cover the top and right edge.
			if (tu == 0.0f) { tu = 1.0f; }
			if (tv == 1.0f) { tv = 0.0f; }

			{
				auto& upperRightPos = heightMap[upperRightIndex];
				auto& upperRightNorm = normalMap[upperRightIndex];
				vertices.emplace_back(PosNormTexVertex(upperRightPos, upperRightNorm, { tu, tv }));
			}

			// Bottom left.
			{
				auto& bottomLeftPos = heightMap[bottomLeftIndex];
				auto& bottomLeftNorm = normalMap[bottomLeftIndex];
				auto& bottomLeftUV = texCoordsMap[bottomLeftIndex];
				vertices.emplace_back(PosNormTexVertex(bottomLeftPos, bottomLeftNorm, bottomLeftUV));
			}
			// Bottom left.
			{
				auto& bottomLeftPos = heightMap[bottomLeftIndex];
				auto& bottomLeftNorm = normalMap[bottomLeftIndex];
				auto& bottomLeftUV = texCoordsMap[bottomLeftIndex];
				vertices.emplace_back(PosNormTexVertex(bottomLeftPos, bottomLeftNorm, bottomLeftUV));
			}

			// Upper right.
			tu = texCoordsMap[upperRightIndex].x;
			tv = texCoordsMap[upperRightIndex].y;

			// Modify the texture coordinates to cover the top and right edge.
			if (tu == 0.0f) { tu = 1.0f; }
			if (tv == 1.0f) { tv = 0.0f; }

			{
				auto& upperRightPos = heightMap[upperRightIndex];
				auto& upperRightNorm = normalMap[upperRightIndex];
				vertices.emplace_back(PosNormTexVertex(upperRightPos, upperRightNorm, { tu, tv }));
			}

			// Bottom right.
			tu = texCoordsMap[bottomRightIndex].x;

			// Modify the texture coordinates to cover the right edge.
			if (tu == 0.0f) { tu = 1.0f; }

			{
				auto& bottomRightPos = heightMap[bottomRightIndex];
				auto& bottomRightNorm = normalMap[bottomRightIndex];
				auto& bottomRightUV = texCoordsMap[bottomRightIndex];
				vertices.emplace_back(PosNormTexVertex(bottomRightPos, bottomRightNorm, { tu, bottomRightUV.y }));
			}
		}
	}

	m_terrainMesh = std::make_unique<QuadTree<PosNormTexVertex>>();
	m_terrainMesh->Generate(commandList, vertices);
}

void Terrain::LoadNormalizedHeightMap(const TerrainInfo& info, std::vector<DirectX::XMFLOAT3>& heightMap, int& terrainWidth, int& terrainHeight)
{
	// Calculate the number of vertices in the terrain mesh.
	std::vector<char> heightBitmap = {};
	helpers::LoadBitmap(info.heightMapPath.c_str(), heightBitmap, terrainWidth, terrainHeight);

	int terrainWidthBorder = terrainWidth - 1;
	int terrainHeightBorder = terrainHeight - 1;

	int heightMapSize = terrainWidth * terrainHeight;
	heightMap.resize(heightMapSize);

	unsigned char heightData;
	int index = 0;
	int bitmapIndex = 0;
	float invNormCoef = 1.f / info.normalizeHeightMapCoef;

	auto get_height_data = [&]() -> unsigned char
	{
		auto& data = heightBitmap[bitmapIndex];
		bitmapIndex += 3;
		return data;
	};

	for (int j = 0; j < terrainHeight; ++j)
	{
		for (int i = 0; i < terrainWidth; ++i)
		{
			heightData = get_height_data();

			index = terrainHeight * j + i;
			heightMap[index].x = static_cast<float>(i);
			heightMap[index].y = static_cast<float>(heightData);
			heightMap[index].z = static_cast<float>(j);

			//normalization
			heightMap[index].y *= invNormCoef;
		}
	}
}

void Terrain::CalculateNormals(std::vector<DirectX::XMFLOAT3>& normalMap, const std::vector<DirectX::XMFLOAT3>& heightMap, int terrainWidth, int terrainHeight)
{
	int terrainWidthBorder = terrainWidth - 1;
	int terrainHeightBorder = terrainHeight - 1;
	int normalMapSize = terrainWidth * terrainHeight;
	normalMap.resize(normalMapSize);
	int normalsSize = terrainWidthBorder * terrainHeightBorder;
	std::vector<DirectX::XMFLOAT3> normals(normalsSize);
	// Go through all the faces in the mesh and calculate their normals.
	int bottomLeftIndex, bottomRightIndex,
		upperLeftIndex, upperRightIndex;

	for (int j = 0; j < terrainHeightBorder; ++j)
	{
		for (int i = 0; i < terrainWidthBorder; ++i)
		{
			bottomLeftIndex = terrainHeight * j + i;
			bottomRightIndex = terrainHeight * j + i + 1;
			upperLeftIndex = terrainHeight * (j + 1) + i;

			auto& bottomLeftPos = heightMap[bottomLeftIndex];
			auto& bottomRightPos = heightMap[bottomRightIndex];
			auto& upperLeftPos = heightMap[upperLeftIndex];

			auto sideA = Math::float3Substruct(bottomLeftPos, upperLeftPos);
			auto sideB = Math::float3Substruct(upperLeftPos, bottomRightPos);

			int normalIndex = j * terrainHeightBorder + i;
			normals[normalIndex] = Math::float3Cross(sideA, sideB);
		}
	}

	// Now go through all the vertices and take an average of each face normal 	
	// that the vertex touches to get the averaged normal for that vertex.

	for (int j = 0; j < terrainHeight; ++j)
	{
		for (int i = 0; i < terrainWidth; ++i)
		{
			DirectX::XMFLOAT3 normal(0.f, 0.f, 0.f);

			bottomLeftIndex = terrainHeightBorder * (j - 1) + (i - 1);
			bottomRightIndex = terrainHeightBorder * (j - 1) + i;
			upperLeftIndex = terrainHeightBorder * j + (i - 1);
			upperRightIndex = terrainHeightBorder * j + i;

			// Initialize the count.
			int count = 0;

			// Bottom left face.
			if (((i - 1) >= 0) && ((j - 1) >= 0))
			{
				Math::float3Add(normal, normals[bottomLeftIndex]);
				count++;
			}

			// Bottom right face.
			if ((i < terrainWidthBorder) && ((j - 1) >= 0))
			{
				Math::float3Add(normal, normals[bottomRightIndex]);
				count++;
			}

			// Upper left face.
			if (((i - 1) >= 0) && (j < terrainHeightBorder))
			{
				Math::float3Add(normal, normals[upperLeftIndex]);
				count++;
			}

			// Upper right face.
			if ((i < terrainWidthBorder) && (j < terrainHeightBorder))
			{
				Math::float3Add(normal, normals[upperRightIndex]);
				count++;
			}

			// Take the average of the faces touching this vertex.
			Math::float3Div(normal, count);

			// Normalize the final shared normal for this vertex and store it in the height map array.
			Math::float3Normalized(normal);

			int normalMapIndex = j * terrainHeight + i;
			normalMap[normalMapIndex] = normal;
		}
	}
}

void Terrain::CalculateTextureCoordinates(const TerrainInfo& info, std::vector<DirectX::XMFLOAT2>& texCoordMap, int terrainWidth, int terrainHeight)
{
	int texCoordMapSize = terrainWidth * terrainHeight;
	texCoordMap.resize(texCoordMapSize);

	float textureRepeat = static_cast<float>(info.textureRepeat);

	// Calculate how much to increment the texture coordinates by.
	float incrementValue = textureRepeat / terrainWidth;
	// Calculate how many times to repeat the texture.
	int incrementCount = terrainWidth / info.textureRepeat;

	// Initialize the tu and tv coordinate values.
	float tuCoordinate = 0.0f;
	float tvCoordinate = 1.0f;

	// Initialize the tu and tv coordinate indexes.
	int tuCount = 0, tvCount = 0;

	for (int j = 0; j < terrainHeight; j++)
	{
		for (int i = 0; i < terrainWidth; i++)
		{
			int texCoordIndex = terrainHeight * j + i;
			texCoordMap[texCoordIndex].x = tuCoordinate;
			texCoordMap[texCoordIndex].y = tvCoordinate;

			// Increment the tu texture coordinate by the increment value and increment the index by one.
			tuCoordinate += incrementValue;
			tuCount++;

			// Check if at the far right end of the texture and if so then start at the beginning again.
			if (tuCount == incrementCount)
			{
				tuCoordinate = 0.0f;
				tuCount = 0;
			}
		}

		// Increment the tv texture coordinate by the increment value and increment the index by one.
		tvCoordinate -= incrementValue;
		tvCount++;

		// Check if at the top of the texture and if so then start at the bottom again.
		if (tvCount == incrementCount)
		{
			tvCoordinate = 1.0f;
			tvCount = 0;
		}
	}
}

void Terrain::Render(std::shared_ptr<CommandList>& commandList)
{
	m_terrainMesh->Render(commandList);
}

void Terrain::Render(std::shared_ptr<CommandList>& commandList, Frustum& frustum)
{
	m_terrainMesh->Render(commandList, frustum);
}