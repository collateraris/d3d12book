#pragma once

#include <URootObject.h>
#include <Mesh.h>
#include <CommandList.h>
#include <BoundingVolumesPrimitive.h>
#include <Frustum.h>

#include <array>
#include <vector>
#include <cassert>

namespace dx12demo::core
{
	/* VerticesContainer must have DirectX::XMFLOAT3 m_position; 
	*  also compatible with Mesh::CreateCustomMesh method;
	*  member and
	*  overrided copy operator
	*  see examples in mesh.h struct PosNormTexExtendedVertex and 
	*  struct PosNormTexVertex
	*/
	template<typename VerticesContainer>
	class QuadTree : public URootObject
	{
		static const int CHILD_IN_PARENT_NODE = 4;

		struct QuadTreeNode
		{
			QuadTreeNode() = default;

			float posX = 0, posZ = 0;
			float width = 0;
			int triangleCount = 0;
			std::unique_ptr<Mesh> mesh = nullptr;
			std::array<QuadTreeNode*, CHILD_IN_PARENT_NODE> childNodes = {nullptr, nullptr, nullptr, nullptr};
		};

		using NodesStorage = std::vector<QuadTreeNode*>;

	public:
		QuadTree();
		virtual ~QuadTree();

		void Generate(CommandList& commandList, const std::vector<VerticesContainer>& srcVertices, int maxTrianglesInNode = 10000);
		
		void Render(std::shared_ptr<CommandList>& commandList);

		void Render(std::shared_ptr<CommandList>& commandList, Frustum& frustum);

	private:

		void CalculateMeshDimensions(float& centerX, float& centerZ, float& meshWidth);

		void CreateTreeNode(CommandList& commandList, QuadTreeNode*& node, float positionX, float positionZ, float width);

		int CountTriangles(QuadTreeNode* node);

		bool IsTriangleContainedInSourceArea(int indexInSrcVertices, QuadTreeNode* node);

		void SplitNode(CommandList& commandList, QuadTreeNode*& node);

		void CreateMeshForNode(CommandList& commandList, QuadTreeNode*& node);

		void AllocateNode(QuadTreeNode*& node);

		void RenderNode(std::shared_ptr<CommandList>& commandList, const std::array<DirectX::XMFLOAT4, 6>& frustum_planes, QuadTreeNode*& node);

		NodesStorage m_nodes;

		QuadTreeNode* m_rootNode = nullptr;

		const std::vector<VerticesContainer>* m_sourceVertices;

		int m_maxTrianglesInNode;

		int m_triangleCount;
	};

	template<typename VerticesContainer>
	QuadTree<VerticesContainer>::QuadTree()
	{

	}

	template<typename VerticesContainer>
	QuadTree<VerticesContainer>::~QuadTree()
	{
		for (auto& node: m_nodes)
		{
			delete node;
		}

		m_nodes.clear();
	}

	template<typename VerticesContainer>
	void QuadTree<VerticesContainer>::AllocateNode(QuadTreeNode*& node)
	{
		m_nodes.emplace_back(new QuadTreeNode());
		node = m_nodes.back();
	}

	template<typename VerticesContainer>
	void QuadTree<VerticesContainer>::Generate(CommandList& commandList, const std::vector<VerticesContainer>& srcVertices, int maxTrianglesInNode/* = 10000*/)
	{
		m_sourceVertices = &srcVertices;
		m_maxTrianglesInNode = maxTrianglesInNode;

		// Get the number of vertices in the terrain vertex array.
		int vertexCount = m_sourceVertices->size();
		assert(vertexCount >= 3);

		// Store the total triangle count for the vertex list.
		m_triangleCount = vertexCount / 3;

		// Calculate the center x,z and the width of the mesh.
		float centerX, centerZ, width;
		CalculateMeshDimensions(centerX, centerZ, width);

		CreateTreeNode(commandList, m_rootNode, centerX, centerZ, width);
	}

	template<typename VerticesContainer>
	void QuadTree<VerticesContainer>::CalculateMeshDimensions(float& centerX, float& centerZ, float& meshWidth)
	{
		centerX = 0.0f;
		centerZ = 0.0f;

		const auto& vertexList = (*m_sourceVertices);

		// Sum all the vertices in the mesh.
		for (const auto& container: vertexList)
		{
			centerX += container.m_position.x;
			centerZ += container.m_position.z;
		}

		// And then divide it by the number of vertices to find the mid-point of the mesh.
		int vertexCount = vertexList.size();
		centerX = centerX / vertexCount;
		centerZ = centerZ / vertexCount;

		// Initialize the maximum and minimum size of the mesh.
		float maxWidth = 0.0f;
		float maxDepth = 0.0f;

		float minWidth = fabsf(vertexList[0].m_position.x - centerX);
		float minDepth = fabsf(vertexList[0].m_position.z - centerZ);

		// Go through all the vertices and find the maximum and minimum width and depth of the mesh.
		for (const auto& container : vertexList)
		{
			float width = fabsf(container.m_position.x - centerX);
			float depth = fabsf(container.m_position.z - centerZ);

			if (width > maxWidth) { maxWidth = width; }
			if (depth > maxDepth) { maxDepth = depth; }
			if (width < minWidth) { minWidth = width; }
			if (depth < minDepth) { minDepth = depth; }
		}

		// Find the absolute maximum value between the min and max depth and width.
		float maxX = max(std::fabs(minWidth), std::fabs(maxWidth));
		float maxZ = max(std::fabs(minDepth), std::fabs(maxDepth));

		// Calculate the maximum diameter of the mesh.
		meshWidth = max(maxX, maxZ) * 2.0f;
	}

	template<typename VerticesContainer>
	void QuadTree<VerticesContainer>::CreateTreeNode(CommandList& commandList, QuadTreeNode*& node, float positionX, float positionZ, float width)
	{
		AllocateNode(node);
		node->posX = positionX;
		node->posZ = positionZ;
		node->width = width;

		int triangleCount = CountTriangles(node);
		node->triangleCount = triangleCount;

		// Case 1: If there are no triangles in this node then return as it is empty and requires no processing.
		if (triangleCount <= 0)
			return;

		// Case 2: If there are too many triangles in this node then split it into four equal sized smaller tree nodes.
		if (triangleCount > m_maxTrianglesInNode)
		{
			SplitNode(commandList, node);
			return;
		}

		// Case 3: If this node is not empty and the triangle count for it is less than the max then 
		// this node is at the bottom of the tree so create the list of triangles to store in it.
		CreateMeshForNode(commandList, node);
	}

	template<typename VerticesContainer>
	int QuadTree<VerticesContainer>::CountTriangles(QuadTreeNode* node)
	{
		int result = 0;

		for (int i = 0; i < m_triangleCount; i++)
		{
			// If the triangle is inside the node then increment the count by one.
			if (IsTriangleContainedInSourceArea(i, node))
			{
				result++;
			}
		}

		return result;
	}

	template<typename VerticesContainer>
	bool QuadTree<VerticesContainer>::IsTriangleContainedInSourceArea(int indexInSrcVertices, QuadTreeNode* node)
	{
		// Calculate the radius of this node.
		int radius = node->width / 2.0f;

		int positionX = node->posX;
		int positionZ = node->posZ;

		// Get the index into the vertex list.
		int vertexIndex = indexInSrcVertices * 3;

		const auto& vertexList = (*m_sourceVertices);

		// Get the three vertices of this triangle from the vertex list.
		int x1 = vertexList[vertexIndex].m_position.x;
		int z1 = vertexList[vertexIndex].m_position.z;
		vertexIndex++;

		int x2 = vertexList[vertexIndex].m_position.x;
		int z2 = vertexList[vertexIndex].m_position.z;
		vertexIndex++;

		int x3 = vertexList[vertexIndex].m_position.x;
		int z3 = vertexList[vertexIndex].m_position.z;

		// Check to see if the minimum of the x coordinates of the triangle is inside the node.
		int minimumX = min(x1, min(x2, x3));
		if (minimumX > (positionX + radius))
		{
			return false;
		}

		// Check to see if the maximum of the x coordinates of the triangle is inside the node.
		int maximumX = max(x1, max(x2, x3));
		if (maximumX < (positionX - radius))
		{
			return false;
		}

		// Check to see if the minimum of the z coordinates of the triangle is inside the node.
		int minimumZ = min(z1, min(z2, z3));
		if (minimumZ > (positionZ + radius))
		{
			return false;
		}

		// Check to see if the maximum of the z coordinates of the triangle is inside the node.
		int maximumZ = max(z1, max(z2, z3));
		if (maximumZ < (positionZ - radius))
		{
			return false;
		}

		return true;
	}

	template<typename VerticesContainer>
	void QuadTree<VerticesContainer>::SplitNode(CommandList& commandList, QuadTreeNode*& node)
	{
		const float width = node->width;
		const float positionX = node->posX;
		const float positionZ = node->posZ;

		for (int i = 0; i < CHILD_IN_PARENT_NODE; i++)
		{
			// Calculate the position offsets for the new child node.
			float offsetX = (((i % 2) < 1) ? -1.0f : 1.0f) * (width * 0.25f);
			float offsetZ = (((i % 4) < 2) ? -1.0f : 1.0f) * (width * 0.25f);

			QuadTreeNode tmpNode;
			tmpNode.posX = positionX + offsetX;
			tmpNode.posZ = positionZ + offsetZ;
			tmpNode.width = width * 0.5f;

			// See if there are any triangles in the new node.
			if (CountTriangles(&tmpNode) > 0)
			{
				// If there are triangles inside where this new node would be then create the child node.
				// Extend the tree starting from this new child node now.
				CreateTreeNode(commandList, node->childNodes[i], tmpNode.posX, tmpNode.posZ, tmpNode.width);
			}
		}
	}

	template<typename VerticesContainer>
	void QuadTree<VerticesContainer>::CreateMeshForNode(CommandList& commandList, QuadTreeNode*& node)
	{
		// Calculate the number of vertices.
		int vertexCount = node->triangleCount * 3;

		std::vector<VerticesContainer> vertices(vertexCount);
		IndexCollection indices(vertexCount);
		const auto& vertexList = (*m_sourceVertices);

		int vertexStoreIndex = 0;

		CollectorBVData collectorBVData;

		// Go through all the triangles in the vertex list.
		for (int i = 0; i < m_triangleCount; i++)
		{
			if (IsTriangleContainedInSourceArea(i, node))
			{
				// Calculate the index into the terrain vertex list.
				int vertexIndex = i * 3;

				// Get the three vertices of this triangle from the vertex list.
				vertices[vertexStoreIndex] = vertexList[vertexIndex];
				indices[vertexStoreIndex] = vertexStoreIndex;
				collectorBVData.Collect(vertices[vertexStoreIndex].m_position);
				vertexStoreIndex++;

				vertexIndex++;
				vertices[vertexStoreIndex] = vertexList[vertexIndex];
				indices[vertexStoreIndex] = vertexStoreIndex;
				collectorBVData.Collect(vertices[vertexStoreIndex].m_position);
				vertexStoreIndex++;

				vertexIndex++;
				vertices[vertexStoreIndex] = vertexList[vertexIndex];
				indices[vertexStoreIndex] = vertexStoreIndex;
				collectorBVData.Collect(vertices[vertexStoreIndex].m_position);
				vertexStoreIndex++;
			}
		}

		vertices.resize(vertexStoreIndex);

		MeshCreatorInfo info;
		info.bv_min_pos = collectorBVData.GetMin();
		info.bv_max_pos = collectorBVData.GetMax();
		info.bv_pos = collectorBVData.GetCenter();
		info.rhcoords = true;
		info.scale = 1;

		node->mesh = Mesh::CreateCustomMesh(commandList, vertices, indices, info);
	}

	template<typename VerticesContainer>
	void QuadTree<VerticesContainer>::Render(std::shared_ptr<CommandList>& commandList)
	{
		for (auto& node : m_nodes)
		{
			if (node->mesh)
				node->mesh->Render(commandList);
		}
	}

	template<typename VerticesContainer>
	void QuadTree<VerticesContainer>::Render(std::shared_ptr<CommandList>& commandList, Frustum& frustum)
	{		
		RenderNode(commandList, frustum.GetFrustumPlanesF4(), m_rootNode);
	}

	template<typename VerticesContainer>
	void QuadTree<VerticesContainer>::RenderNode(std::shared_ptr<CommandList>& commandList, const std::array<DirectX::XMFLOAT4, 6>& frustum_planes, QuadTreeNode*& node)
	{
		BSphere sphere;
		sphere.r = node->width * 0.5f;
		sphere.pos = { node->posX, 0.0f, node->posZ};

		if (!Frustum::FrustumInSphere(sphere, frustum_planes))
			return;

		bool existChildNodes = false;
		for (int i = 0; i < CHILD_IN_PARENT_NODE; i++)
		{
			if (node->childNodes[i] != 0)
			{
				existChildNodes = true;
				RenderNode(commandList, frustum_planes, node->childNodes[i]);
			}
		}

		if (existChildNodes)
			return;

		node->mesh->Render(commandList);
	}

}