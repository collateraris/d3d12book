#pragma once

#include <Resource.h>
#include "DescriptorAllocation.h"

namespace dx12demo::core
{
	class DepthBuffer : public Resource
	{
	public:

		explicit DepthBuffer(const std::wstring& name = L"");
		explicit DepthBuffer(const D3D12_RESOURCE_DESC& resourceDesc,
			const D3D12_CLEAR_VALUE* clearValue,
			const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc,
			const D3D12_DEPTH_STENCIL_VIEW_DESC& dsvDesc,
			const std::wstring& name = L"");

		virtual ~DepthBuffer();

		virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const override;

		virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;

	private:

		DescriptorAllocation m_SRV;
		DescriptorAllocation m_DSV;
		
	};
}