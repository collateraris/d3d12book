#include <DepthBuffer.h>

#include <Application.h>

using namespace dx12demo::core;

DepthBuffer::DepthBuffer(const std::wstring& name/* = L""*/)
	:Resource(name)
{
}

DepthBuffer::DepthBuffer(const D3D12_RESOURCE_DESC& resourceDesc,
	const D3D12_CLEAR_VALUE* clearValue,
	const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc,
	const D3D12_DEPTH_STENCIL_VIEW_DESC& dsvDesc,
	const std::wstring& name/* = L""*/)
	: Resource(resourceDesc, clearValue, name)
{
	m_SRV = GetApp().AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_DSV = GetApp().AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	auto& device = GetApp().GetDevice();

	device->CreateShaderResourceView(m_d3d12Resource.Get(),
		&srvDesc,
		m_SRV.GetDescriptorHandle());

	device->CreateDepthStencilView(m_d3d12Resource.Get(),
		&dsvDesc,
		m_DSV.GetDescriptorHandle());
}

DepthBuffer::~DepthBuffer()
{

}

D3D12_CPU_DESCRIPTOR_HANDLE DepthBuffer::GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc/* = nullptr*/) const
{
	return m_SRV.GetDescriptorHandle();
}

D3D12_CPU_DESCRIPTOR_HANDLE DepthBuffer::GetDepthStencilView() const
{
	return m_DSV.GetDescriptorHandle();
}