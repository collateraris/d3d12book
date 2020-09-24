#pragma once

#include <URootObject.h>

#include <memory>
#include <string>
#include <vector>
#include <functional>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class aiMaterial;
class aiMesh;
class aiNode;
class aiScene;
enum aiTextureType;

namespace dx12demo::core
{
	class Mesh;
	class Material;
	class CommandList;

	class Scene : public URootObject
	{
	public:
		Scene();
		virtual ~Scene();

		bool LoadFromFile(std::shared_ptr<CommandList>& commandList, const std::wstring& fileName, bool rhcoords = false);
		void Render(std::shared_ptr<CommandList>& commandList, std::function<void(std::shared_ptr<Material>&)>& drawMatFun);

	private:

		void ProcessNode(std::shared_ptr<CommandList>& commandList, aiNode* node, const aiScene* scene);

		void ProcessMesh(std::shared_ptr<CommandList>& commandList, aiMesh* mesh, const aiScene* scene);

		void ProcessMeshLoadMaterialTextures(std::shared_ptr<CommandList>& commandList, std::shared_ptr<Material>& inStoreMat, aiMaterial* mat, aiTextureType type);

		using MeshMaterialList = std::vector<std::pair<std::shared_ptr<Mesh>, std::shared_ptr<Material>>>;

		MeshMaterialList m_Data;

		std::string m_lastDirectory;
		bool m_last_rhcoords = false;
	};

}