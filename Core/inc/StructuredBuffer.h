#pragma once

#include "Buffer.h"
#include "ByteAddressBuffer.h"

namespace dx12demo::core
{
	class StructuredBuffer : public Buffer
	{
	public:
		StructuredBuffer(const std::wstring& name = L"");
		StructuredBuffer(const D3D12_RESOURCE_DESC& resDesc,
			size_t numElements, size_t elementSize,
			const std::wstring& name = L"");

		virtual ~StructuredBuffer();

		virtual size_t GetNumElements() const;
		virtual size_t GetElementSize() const;

		virtual void CreateViews(size_t numElements = 0, size_t elementSize = 0) override;

        virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const;

		virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const override;

		const ByteAddressBuffer& GetCounterBuffer() const;

	private:
		size_t m_NumElements;
		size_t m_ElementSize;

		DescriptorAllocation m_SRV;
		DescriptorAllocation m_UAV;

		// A buffer to store the internal counter for the structured buffer.
		ByteAddressBuffer m_CounterBuffer;
	};
}