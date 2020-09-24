#pragma once

#include <URootObject.h>

#include <memory>
#include <string>
#include <map>
#include <vector>

#include <DirectXMath.h>

namespace dx12demo::core
{
	class Mesh;
	class CommandList;

	class SceneNode : public URootObject, public std::enable_shared_from_this<SceneNode>
	{
	public:
		explicit SceneNode(const DirectX::XMMATRIX& localTransform = DirectX::XMMatrixIdentity());
		virtual ~SceneNode();

		const std::string& GetName() const;
		void SetName(const std::string& name);

		const DirectX::XMMATRIX& GetLocalTransform() const;
		void SetLocalTransform(const DirectX::XMMATRIX& localTransform);

		const DirectX::XMMATRIX& GetInverseLocalTransform() const;

		const DirectX::XMMATRIX& GetWorldTransform() const;
		void SetWorldTransform(const DirectX::XMMATRIX& worldTransform);

		const DirectX::XMMATRIX& GetInverseWorldTransform() const;

		void AddChild(std::shared_ptr<SceneNode>& childNode);
		void RemoveChild(std::shared_ptr<SceneNode>& childNode);
		void SetParent(std::weak_ptr<SceneNode>& parentNode);

		void AddMesh(std::shared_ptr<Mesh>& mesh);
		void RemoveMesh(std::shared_ptr<Mesh>& mesh);

		void Render(CommandList& commandList);

	protected:
		const DirectX::XMMATRIX& GetParentWorldTransform() const;

	private:
		using NodePtr = std::shared_ptr<SceneNode>;
		using NodeList = std::vector<NodePtr>;
		using NodeNameMap = std::multimap<std::string, NodePtr>;
		using MeshList = std::vector<std::shared_ptr<Mesh>>;

		std::string m_Name;

		// This data must be aligned to a 16-byte boundary. 
		// The only way to guarantee this, is to allocate this
		// structure in aligned memory.
		struct alignas(16) AlignedData
		{
			DirectX::XMMATRIX m_LocalTransform;
			DirectX::XMMATRIX m_InverseTransform;
		} *m_AlignedData;

		std::weak_ptr<SceneNode> m_ParentNode;
		NodeList m_Children;
		NodeNameMap m_ChildrenByName;
		MeshList m_Meshes;
	};
}