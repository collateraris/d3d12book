#pragma once

#include "Buffer.h"
#include "DescriptorAllocation.h"

namespace dx12demo::core
{
	class ConstantBuffer : public Buffer
	{
	public:
		ConstantBuffer(const std::wstring& name = L"");
		virtual ~ConstantBuffer();

		size_t GetSizeInBytes() const;

		virtual void CreateViews(size_t numElements, size_t elementSize) override;

        D3D12_CPU_DESCRIPTOR_HANDLE GetConstantBufferView() const;

        /**
        * Get the SRV for a resource.
        */
        virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const override;

        /**
        * Get the UAV for a (sub)resource.
        */
        virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const override;

    private:
        size_t m_SizeInBytes;
        DescriptorAllocation m_ConstantBufferView;

	};
}