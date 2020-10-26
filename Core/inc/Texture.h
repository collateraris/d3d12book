#pragma once

#include <Resource.h>
#include <DescriptorAllocation.h>
#include <TextureUsage.h>

#include "d3dx12.h"

#include <mutex>
#include <unordered_map>
#include <optional>

namespace dx12demo::core
{
	class Texture : public Resource
	{
    public:
        explicit Texture(TextureUsage textureUsage = TextureUsage::None,
            const std::wstring& name = L"");
        explicit Texture(Microsoft::WRL::ComPtr<ID3D12Resource> resource,
            TextureUsage textureUsage = TextureUsage::None,
            const std::wstring& name = L"");
        explicit Texture(const D3D12_RESOURCE_DESC& resourceDesc,
            const D3D12_CLEAR_VALUE* clearValue = nullptr,
            TextureUsage textureUsage = TextureUsage::None,
            const std::wstring& name = L"",
            const D3D12_RENDER_TARGET_VIEW_DESC* rtvDesc = nullptr,
            const D3D12_DEPTH_STENCIL_VIEW_DESC* dsvDesc = nullptr);

        Texture(const Texture& copy);
        Texture(Texture&& copy);

        Texture& operator=(const Texture& other);
        Texture& operator=(Texture&& other);

        virtual ~Texture();

        const TextureUsage& GetTextureUsage() const;

        bool IsEmpty() const;

        void SetTextureUsage(const TextureUsage& textureUsage);

        /**
         * Resize the texture.
         */
        void Resize(uint32_t width, uint32_t height, uint32_t depthOrArraySize = 1);

        /**
         * Create SRV and UAVs for the resource.
         */
        virtual void CreateViews(const D3D12_RENDER_TARGET_VIEW_DESC* rtvDesc = nullptr, const D3D12_DEPTH_STENCIL_VIEW_DESC* dsvDesc = nullptr);

        /**
        * Get the SRV for a resource.
        */
        virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const override;

        /**
        * Get the UAV for a (sub)resource.
        */
        virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const override;

        virtual D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView() const;
        virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;

        bool CheckSRVSupport() const;

        bool CheckRTVSupport() const;

        bool CheckUAVSupport() const;

        bool CheckDSVSupport() const;

        bool IsUAVCompatibleFormat();

        static bool IsUAVCompatibleFormat(DXGI_FORMAT format);
        static bool IsSRGBFormat(DXGI_FORMAT format);
        static bool IsBGRFormat(DXGI_FORMAT format);
        static bool IsDepthFormat(DXGI_FORMAT format);

        // Return a typeless format from the given format.
        static DXGI_FORMAT GetTypelessFormat(DXGI_FORMAT format);
        static DXGI_FORMAT GetSRGBFormat(DXGI_FORMAT format);
        static DXGI_FORMAT GetUAVCompatableFormat(DXGI_FORMAT format);

    private:

        DescriptorAllocation CreateShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc) const;
        DescriptorAllocation CreateUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc) const;

        DescriptorAllocation m_RenderTargetView;
        DescriptorAllocation m_DepthStencilView;

        mutable std::unordered_map<size_t, DescriptorAllocation> m_ShaderResourceViews;
        mutable std::unordered_map<size_t, DescriptorAllocation> m_UnorderedAccessViews;

        mutable std::mutex m_ShaderResourceViewsMutex;
        mutable std::mutex m_UnorderedAccessViewsMutex;

        TextureUsage m_TextureUsage = TextureUsage::None;
        
        std::optional<D3D12_RENDER_TARGET_VIEW_DESC> m_RTV;
        std::optional<D3D12_DEPTH_STENCIL_VIEW_DESC> m_DSV;
	};
}