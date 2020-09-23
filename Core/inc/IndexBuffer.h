#pragma once

#include "Buffer.h"

namespace dx12demo::core
{
    class IndexBuffer : public Buffer
    {
    public:
        IndexBuffer(const std::wstring& name = L"");
        virtual ~IndexBuffer();

        virtual void CreateViews(size_t numElements, size_t elementSize) override;

        size_t GetNumIndicies() const;

        DXGI_FORMAT GetIndexFormat() const;

        /**
         * Get the index buffer view for biding to the Input Assembler stage.
         */
        D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const;

        /**
        * Get the SRV for a resource.
        */
        virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const override;

        /**
        * Get the UAV for a (sub)resource.
        */
        virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const override;

    private:
        size_t m_NumIndicies;
        DXGI_FORMAT m_IndexFormat;

        D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;
    };
}