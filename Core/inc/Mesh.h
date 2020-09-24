#pragma once

#include <CommandList.h>
#include <VertexBuffer.h>
#include <IndexBuffer.h>

#include <DirectXMath.h>
#include <d3d12.h>

#include <wrl.h>

#include <memory> // For std::unique_ptr
#include <vector>

namespace dx12demo::core
{
	// Vertex struct holding position, normal vector, and texture mapping information.
	struct PosNormTexTangBitangVertex
	{
        PosNormTexTangBitangVertex()
        { }

        PosNormTexTangBitangVertex(const DirectX::XMFLOAT3& position,
            const DirectX::XMFLOAT3& normal, 
            const DirectX::XMFLOAT2& textureCoordinate,
            const DirectX::XMFLOAT3& tangent = { 0.f, 0.f, 0.f },
            const DirectX::XMFLOAT3& bitangent = { 0.f, 0.f, 0.f })
            : m_position(position),
            m_normal(normal),
            m_texCoord(textureCoordinate),
            m_tangent(tangent),
            m_bitangent(bitangent)

        { }

        PosNormTexTangBitangVertex(const DirectX::FXMVECTOR& position,
            const DirectX::FXMVECTOR& normal,
            const DirectX::FXMVECTOR& textureCoordinate,
            const DirectX::FXMVECTOR& tangent = { 0.f, 0.f, 0.f },
            const DirectX::FXMVECTOR& bitangent = { 0.f, 0.f, 0.f })
        {
            XMStoreFloat3(&this->m_position, position);
            XMStoreFloat3(&this->m_normal, normal);
            XMStoreFloat2(&this->m_texCoord, textureCoordinate);
            XMStoreFloat3(&this->m_tangent, tangent);
            XMStoreFloat3(&this->m_bitangent, bitangent);
        }

        DirectX::XMFLOAT3 m_position;
        DirectX::XMFLOAT3 m_normal;
        DirectX::XMFLOAT2 m_texCoord;
        DirectX::XMFLOAT3 m_tangent;
        DirectX::XMFLOAT3 m_bitangent;

        static const int InputElementCount = 3;
        static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];

        static const int InputElementCountExtended = 5;
        static const D3D12_INPUT_ELEMENT_DESC InputElementsExtended[InputElementCountExtended];
	};

    using VertexCollection = std::vector<PosNormTexTangBitangVertex>;
    using IndexCollection = std::vector<uint16_t>;

    class Mesh
    {
    public:

        void Render(CommandList& commandList, uint32_t instanceCount = 1, uint32_t firstInstance = 0);

        static std::unique_ptr<Mesh> CreateCustomMesh(CommandList& commandList, VertexCollection& vertices, IndexCollection& indices, bool rhcoords = false);
        static std::unique_ptr<Mesh> CreateCube(CommandList& commandList, float size = 1, bool rhcoords = false);
        static std::unique_ptr<Mesh> CreateSphere(CommandList& commandList, float diameter = 1, size_t tessellation = 16, bool rhcoords = false);
        static std::unique_ptr<Mesh> CreateCone(CommandList& commandList, float diameter = 1, float height = 1, size_t tessellation = 32, bool rhcoords = false);
        static std::unique_ptr<Mesh> CreateTorus(CommandList& commandList, float diameter = 1, float thickness = 0.333f, size_t tessellation = 32, bool rhcoords = false);
        static std::unique_ptr<Mesh> CreatePlane(CommandList& commandList, float width = 1, float height = 1, bool rhcoords = false);

    protected:

    private:
        friend struct std::default_delete<Mesh>;

        Mesh();
        Mesh(const Mesh& copy) = delete;
        virtual ~Mesh();

        void Initialize(CommandList& commandList, VertexCollection& vertices, IndexCollection& indices, bool rhcoords);

        VertexBuffer m_VertexBuffer;
        IndexBuffer m_IndexBuffer;

        UINT m_IndexCount;
    };
}