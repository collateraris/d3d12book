#pragma once

#include <CommandList.h>
#include <Mesh.h>

#include <DirectXMath.h>

#include <fstream>
#include <vector>
#include <memory>

using namespace DirectX;

namespace dx12demo::stdu
{
    enum class SceneRootParameters
    {
        MatricesCB,         // ConstantBuffer<Mat> MatCB : register(b0);
        DirLight,           // StructuredBuffer<PointLight> PointLights : register( b1 );
        Materials,          // ConstantBuffer<Mat> MatCB : register(b2);
        AmbientTex,         // Texture2D AmbientTexture : register( t0 );
        NumRootParameters
    };

    struct __declspec(align(16)) DirLight
    {
        XMFLOAT4 ambientColor;
        XMFLOAT4 strength;
        XMFLOAT3 lightDirection;
    };

    struct __declspec(align(16)) Mat
    {
        XMMATRIX ModelMatrix;
        XMMATRIX ModelViewMatrix;
        XMMATRIX InverseTransposeModelViewMatrix;
        XMMATRIX ModelViewProjectionMatrix;
    };

    struct RenderItem
    {
        int16_t mesh_index = -1;
        bool bUseSubmesh = false;
        int16_t submesh_index = -1;
        int16_t tex_index = -1;
        int16_t mat_index = -1;
        bool bUseCustomWoldMatrix = false;
        DirectX::XMMATRIX worldMatrix = DirectX::XMMatrixScaling(1.f, 1.f, 1.f);
    };

    struct __declspec(align(16)) MaterialConstant
    {
        DirectX::XMFLOAT4 diffuseAlbedo;
        DirectX::XMFLOAT3 freshnelR0;
        float Roughness;
    };

    // from Frank Luna Book
    static std::unique_ptr<core::Mesh> LoadMeshFromFile(core::CommandList& commandList, const std::string& path)
    {
        std::ifstream fin(path.c_str());

        core::VertexCollection vertices;
        core::IndexCollection indices;

        if (!fin)
        {
            assert(false);
        }
        else
        {
            UINT vcount = 0;
            UINT tcount = 0;
            std::string ignore;

            fin >> ignore >> vcount;
            fin >> ignore >> tcount;
            fin >> ignore >> ignore >> ignore >> ignore;

            vertices.resize(vcount);
            for (UINT i = 0; i < vcount; ++i)
            {
                auto& pos = vertices[i].m_position;
                fin >> pos.x >> pos.y >> pos.z;
                auto& normal = vertices[i].m_normal;
                fin >> normal.x >> normal.y >> normal.z;

                // Model does not have texture coordinates, so just zero them out.
                vertices[i].m_texCoord = { 0.0f, 0.0f };
            }

            fin >> ignore;
            fin >> ignore;
            fin >> ignore;

            indices.resize(3 * tcount);
            for (UINT i = 0; i < tcount; ++i)
            {
                fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
            }

            fin.close();
        }

        return core::Mesh::CreateCustomMesh(commandList, vertices, indices, true);
    }

    enum class ERoomPart
    {
        floor = 1,
        wall = 2,
        mirror = 3,
    };

    static std::unique_ptr<core::Mesh> BuildRoomMesh(core::CommandList& commandList)
    {
        core::VertexCollection vertices =
        {
            // Floor: Observe we tile texture coordinates.
            core::PosNormTexVertex(XMFLOAT3(-3.5f, 0.0f, -10.0f), {0.0f, 1.0f, 0.0f}, {0.0f, 4.0f }), // 0 
            core::PosNormTexVertex(XMFLOAT3{-3.5f, 0.0f,   0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}),
            core::PosNormTexVertex(XMFLOAT3{7.5f, 0.0f,   0.0f}, {0.0f, 1.0f, 0.0f}, {4.0f, 0.0f}),
            core::PosNormTexVertex(XMFLOAT3{7.5f, 0.0f, -10.0f}, {0.0f, 1.0f, 0.0f}, {4.0f, 4.0f}),

            // Wall: Observe we tile texture coordinates, and that we
            // leave a gap in the middle for the mirror.
            core::PosNormTexVertex(XMFLOAT3{-3.5f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 2.0f}), // 4
            core::PosNormTexVertex(XMFLOAT3{-3.5f, 4.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}),
            core::PosNormTexVertex(XMFLOAT3{-2.5f, 4.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.5f, 0.0f}),
            core::PosNormTexVertex(XMFLOAT3{-2.5f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.5f, 2.0f}),

            core::PosNormTexVertex(XMFLOAT3{2.5f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 2.0f}), // 8 
            core::PosNormTexVertex(XMFLOAT3{2.5f, 4.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}),
            core::PosNormTexVertex(XMFLOAT3{7.5f, 4.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {2.0f, 0.0f}),
            core::PosNormTexVertex(XMFLOAT3{7.5f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {2.0f, 2.0f}),

            core::PosNormTexVertex(XMFLOAT3{-3.5f, 4.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}), // 12
            core::PosNormTexVertex(XMFLOAT3{-3.5f, 6.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}),
            core::PosNormTexVertex(XMFLOAT3{7.5f, 6.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {6.0f, 0.0f}),
            core::PosNormTexVertex(XMFLOAT3{7.5f, 4.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {6.0f, 1.0f}),

            // Mirror
            core::PosNormTexVertex(XMFLOAT3{-2.5f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}), // 16
            core::PosNormTexVertex(XMFLOAT3{-2.5f, 4.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}),
            core::PosNormTexVertex(XMFLOAT3{2.5f, 4.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}),
            core::PosNormTexVertex(XMFLOAT3{2.5f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f})
        };

        core::IndexCollection indices =
        {
            // Floor
            0, 1, 2,
            0, 2, 3,

            // Walls
            4, 5, 6,
            4, 6, 7,

            8, 9, 10,
            8, 10, 11,

            12, 13, 14,
            12, 14, 15,

            // Mirror
            16, 17, 18,
            16, 18, 19
        };

        auto mesh = core::Mesh::CreateCustomMesh(commandList, vertices, indices, true);

        core::SubMesh floorSubmesh;
        floorSubmesh.IndexCount = 6;
        floorSubmesh.StartIndexLocation = 0;
        floorSubmesh.BaseVertexLocation = 0;
        mesh->PushSubMesh(static_cast<uint16_t>(ERoomPart::floor), floorSubmesh);

        core::SubMesh wallSubmesh;
        wallSubmesh.IndexCount = 18;
        wallSubmesh.StartIndexLocation = 6;
        wallSubmesh.BaseVertexLocation = 0;
        mesh->PushSubMesh(static_cast<uint16_t>(ERoomPart::wall), wallSubmesh);

        core::SubMesh mirrorSubmesh;
        mirrorSubmesh.IndexCount = 6;
        mirrorSubmesh.StartIndexLocation = 24;
        mirrorSubmesh.BaseVertexLocation = 0;
        mesh->PushSubMesh(static_cast<uint16_t>(ERoomPart::mirror), mirrorSubmesh);

        return mesh;
    }
}