#include <Scene.h>

#include <DX12LibPCH.h>

#include <CommandList.h>
#include <Material.h>
#include <Mesh.h>
#include <BoundingVolumesPrimitive.h>
#include <Frustum.h>

#include <DirectXMath.h>

#include <iostream>

using namespace dx12demo::core;

using VertexCollection = std::vector<PosNormTexVertex>;
using IndexCollection = std::vector<uint16_t>;

Scene::Scene()
{

}

Scene::~Scene()
{

}

bool Scene::LoadFromFile(std::shared_ptr<CommandList>& commandList, const std::wstring& fileName, bool rhcoords/* = false*/, float scale/* = 1*/)
{
    fs::path filePath(fileName);
    if (!fs::exists(filePath))
    {
        return false;
    }

    std::string path(fileName.cbegin(), fileName.cend());
    Assimp::Importer import;
    const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
        return false;
    }

    m_lastDirectory = path.substr(0, path.find_last_of('/'));
    m_last_rhcoords = rhcoords;
    m_last_scale = scale;

    ProcessNode(commandList, scene->mRootNode, scene);
    return true;
}

void Scene::ProcessNode(std::shared_ptr<CommandList>& commandList, aiNode* node, const aiScene* scene)
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ProcessMesh(commandList, mesh, scene);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(commandList, node->mChildren[i], scene);
    }
}

void Scene::ProcessMesh(std::shared_ptr<CommandList>& commandList, aiMesh* mesh, const aiScene* scene)
{
    static VertexExtendedCollection vertices;
    static IndexCollection indices;

    vertices.clear();
    indices.clear();

    CollectorBVData collectorBVData;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        DirectX::XMFLOAT3 position;
        position.x = mesh->mVertices[i].x;
        position.y = mesh->mVertices[i].y;
        position.z = mesh->mVertices[i].z;
        collectorBVData.Collect(position);

        DirectX::XMFLOAT3 normal;
        normal.x = mesh->mNormals[i].x;
        normal.y = mesh->mNormals[i].y;
        normal.z = mesh->mNormals[i].z;

        DirectX::XMFLOAT3 tangent;
        tangent.x = mesh->mTangents[i].x;
        tangent.y = mesh->mTangents[i].y;
        tangent.z = mesh->mTangents[i].z;

        DirectX::XMFLOAT3 bitangent;
        bitangent.x = mesh->mBitangents[i].x;
        bitangent.y = mesh->mBitangents[i].y;
        bitangent.z = mesh->mBitangents[i].z;

        DirectX::XMFLOAT2 textureCoordinate;
        if (mesh->mTextureCoords[0])
        {
            textureCoordinate.x = mesh->mTextureCoords[0][i].x;
            textureCoordinate.y = mesh->mTextureCoords[0][i].y;
        }
        else
        {
            textureCoordinate = {0., 0.};
        }
  
        vertices.emplace_back(PosNormTexExtendedVertex(position, normal, textureCoordinate, tangent, bitangent));
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }


    MeshCreatorInfo info;
    info.bv_min_pos = collectorBVData.GetMin();
    info.bv_max_pos = collectorBVData.GetMax();
    info.bv_pos = collectorBVData.GetCenter();
    info.rhcoords = m_last_rhcoords;
    info.scale = m_last_scale;

    std::shared_ptr<Mesh> storedMesh = Mesh::CreateCustomMesh(*commandList, vertices, indices, info);

    std::shared_ptr<Material> meshMaterial(new Material);
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        ProcessMeshLoadMaterialTextures(commandList, meshMaterial, material,
            aiTextureType_DIFFUSE);
        ProcessMeshLoadMaterialTextures(commandList, meshMaterial, material,
            aiTextureType_SPECULAR);
        ProcessMeshLoadMaterialTextures(commandList, meshMaterial, material,
            aiTextureType_HEIGHT);
        ProcessMeshLoadMaterialTextures(commandList, meshMaterial, material,
            aiTextureType_NORMALS);
        ProcessMeshLoadMaterialTextures(commandList, meshMaterial, material,
            aiTextureType_AMBIENT);
    }

    m_Data.emplace_back(std::make_pair(storedMesh, meshMaterial));
}

void Scene::ProcessMeshLoadMaterialTextures(std::shared_ptr<CommandList>& commandList, std::shared_ptr<Material>& inStoreMat, aiMaterial* mat, aiTextureType type)
{
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);
        std::string texPath = m_lastDirectory + "/" + std::string(str.C_Str());
        std::wstring path(texPath.cbegin(), texPath.cend());

        switch (type)
        {
        case aiTextureType_DIFFUSE:
            inStoreMat->LoadDiffuseTex(commandList, path);
            break;
        case aiTextureType_SPECULAR:
            inStoreMat->LoadSpecularTex(commandList, path);
            break;
        case aiTextureType_HEIGHT:
        case aiTextureType_NORMALS:
            inStoreMat->LoadNormalTex(commandList, path);
            break;
        case aiTextureType_AMBIENT:
            inStoreMat->LoadAmbientTex(commandList, path);
            break;
        default:
            continue;
        }
    }
}

void Scene::Render(std::shared_ptr<CommandList>& commandList, std::function<void(std::shared_ptr<Material>&)>& drawMatFun)
{
    for (auto& nextMesh : m_Data)
    {
        auto& mesh = nextMesh.first;
        auto& mat = nextMesh.second;

        drawMatFun(mat);
        mesh->Render(*commandList);
    }
}

void Scene::Render(std::shared_ptr<CommandList>& commandList, Frustum& frustum, std::function<void(std::shared_ptr<Material>&)>& drawMatFun)
{
    for (auto& nextMesh : m_Data)
    {
        auto& mesh = nextMesh.first;
        auto& mat = nextMesh.second;

        if (!Frustum::FrustumInSphere(mesh->GetBSphere(), frustum.GetFrustumPlanesF4()))
            continue;

        drawMatFun(mat);
        mesh->Render(*commandList);
    }
}

