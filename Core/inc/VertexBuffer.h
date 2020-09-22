#pragma once

#include "Buffer.h"

namespace dx12demo::core
{
	class VertexBuffer : public Buffer
	{
	public:
		VertexBuffer(const std::wstring& name = L"");
		virtual ~VertexBuffer();

        // Inherited from Buffer
        virtual void CreateViews(size_t numElements, size_t elementSize) override;

        D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const;

        size_t GetNumVertices() const;

        size_t GetVertexStride() const;

        virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const override;

        virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const override;
	
    private:
        size_t m_NumVertices;
        size_t m_VertexStride;

        D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;

    };
}