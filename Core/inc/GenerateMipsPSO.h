#pragma once

#include <URootObject.h>
#include <RootSignature.h>
#include <DescriptorAllocation.h>

#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl.h>

namespace dx12demo::core
{
    struct alignas(16) GenerateMipsCB
    {
        uint32_t SrcMipLevel;           // Texture level of source mip
        uint32_t NumMipLevels;          // Number of OutMips to write: [1-4]
        uint32_t SrcDimension;          // Width and height of the source texture are even or odd.
        uint32_t IsSRGB;                // Must apply gamma correction to sRGB textures.
        DirectX::XMFLOAT2 TexelSize;    // 1.0 / OutMip1.Dimensions
    };

    namespace GenerateMips
    {
        enum
        {
            GenerateMipsCB,
            SrcMip,
            OutMip,
            NumRootParameters
        };
    }

    class GenerateMipsPSO : public URootObject
    {
    public:
        GenerateMipsPSO();

        const RootSignature& GetRootSignature() const
        {
            return m_RootSignature;
        }

        Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPipelineState() const
        {
            return m_PipelineState;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE GetDefaultUAV() const
        {
            return m_DefaultUAV.GetDescriptorHandle();
        }

    private:
        RootSignature m_RootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;
        // Default (no resource) UAV's to pad the unused UAV descriptors.
        // If generating less than 4 mip map levels, the unused mip maps
        // need to be padded with default UAVs (to keep the DX12 runtime happy).
        DescriptorAllocation m_DefaultUAV;
    };


}