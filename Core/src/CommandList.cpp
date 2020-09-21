#include <CommandList.h>

#include <DX12LibPCH.h>
#include <Application.h>
#include <ResourceStateTracker.h>
#include <UploadBuffer.h>
#include <DynamicDescriptorHeap.h>
#include <Texture.h>
#include <Resource.h>
#include <CommandQueue.h>
#include <GenerateMipsPSO.h>
#include <RootSignature.h>

using namespace dx12demo::core;

CommandList::CommandList(D3D12_COMMAND_LIST_TYPE type)
    : m_d3d12CommandListType(type)
{
    auto& device = GetApp().GetDevice();

    ThrowIfFailed(device->CreateCommandAllocator(m_d3d12CommandListType, IID_PPV_ARGS(&m_d3d12CommandAllocator)));

    ThrowIfFailed(device->CreateCommandList(0, m_d3d12CommandListType, m_d3d12CommandAllocator.Get(),
        nullptr, IID_PPV_ARGS(&m_d3d12CommandList)));

    m_UploadBuffer = std::make_unique<UploadBuffer>();

    m_ResourceStateTracker = std::make_unique<ResourceStateTracker>();

    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        m_DynamicDescriptorHeap[i] = std::make_unique<DynamicDescriptorHeap>(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
        m_DescriptorHeaps[i] = nullptr;
    }
}

CommandList::~CommandList()
{}

void CommandList::TransitionBarrier(const Microsoft::WRL::ComPtr<ID3D12Resource>& resource, D3D12_RESOURCE_STATES stateAfter, UINT subresource, bool flushBarriers)
{
    if (resource)
    {
        // The "before" state is not important. It will be resolved by the resource state tracker.
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), D3D12_RESOURCE_STATE_COMMON, stateAfter, subresource);
        m_ResourceStateTracker->ResourceBarrier(barrier);
    }

    if (flushBarriers)
    {
        FlushResourceBarriers();
    }
}

void CommandList::TransitionBarrier(const Resource& resource, D3D12_RESOURCE_STATES stateAfter, UINT subresource, bool flushBarriers)
{
    TransitionBarrier(resource.GetD3D12Resource(), stateAfter, subresource, flushBarriers);
}

void CommandList::UAVBarrier(const Resource& resource, bool flushBarriers)
{
    auto d3d12Resource = resource.GetD3D12Resource();
    auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(d3d12Resource.Get());

    m_ResourceStateTracker->ResourceBarrier(barrier);

    if (flushBarriers)
    {
        FlushResourceBarriers();
    }
}

void CommandList::AliasingBarrier(const Resource& beforeResource, const Resource& afterResource, bool flushBarriers)
{
    AliasingBarrier(beforeResource.GetD3D12Resource(), afterResource.GetD3D12Resource(), flushBarriers);
}

void CommandList::AliasingBarrier(const Microsoft::WRL::ComPtr<ID3D12Resource>& beforeResource, Microsoft::WRL::ComPtr<ID3D12Resource> afterResource, bool flushBarriers/* = false*/)
{
    auto barrier = CD3DX12_RESOURCE_BARRIER::Aliasing(beforeResource.Get(), afterResource.Get());

    m_ResourceStateTracker->ResourceBarrier(barrier);

    if (flushBarriers)
    {
        FlushResourceBarriers();
    }
}

void CommandList::FlushResourceBarriers()
{
    m_ResourceStateTracker->FlushResourceBarriers(*this);
}

void CommandList::CopyResource(Resource& dstRes, const Resource& srcRes)
{
    CopyResource(dstRes.GetD3D12Resource(), srcRes.GetD3D12Resource());
}

void CommandList::CopyResource(const Microsoft::WRL::ComPtr<ID3D12Resource>& dstRes, const Microsoft::WRL::ComPtr<ID3D12Resource>& srcRes)
{
    TransitionBarrier(dstRes, D3D12_RESOURCE_STATE_COPY_DEST);
    TransitionBarrier(srcRes, D3D12_RESOURCE_STATE_COPY_SOURCE);

    FlushResourceBarriers();

    m_d3d12CommandList->CopyResource(dstRes.Get(), srcRes.Get());

    TrackObject(dstRes);
    TrackObject(srcRes);
}

void CommandList::TrackObject(const Microsoft::WRL::ComPtr<ID3D12Object>& object)
{
    m_TrackedObjects.push_back(object);
}

void CommandList::TrackResource(const Resource& res)
{
    TrackObject(res.GetD3D12Resource());
}

void CommandList::ReleaseTrackedObjects()
{
    m_TrackedObjects.clear();
}

void CommandList::SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, size_t sizeInBytes, const void* bufferData)
{
    // Constant buffers must be 256-byte aligned.
    auto heapAllococation = m_UploadBuffer->Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    memcpy(heapAllococation.CPU, bufferData, sizeInBytes);

    m_d3d12CommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, heapAllococation.GPU);
}

void CommandList::SetShaderResourceView(
    uint32_t rootParameterIndex,
    uint32_t descriptorOffset,
    const Resource& resource,
    D3D12_RESOURCE_STATES stateAfter/* = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE*/,
    UINT firstSubresource/* = 0*/,
    UINT numSubresources/* = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/,
    const D3D12_SHADER_RESOURCE_VIEW_DESC* srv/* = nullptr*/
)
{
    if (numSubresources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
    {
        for (uint32_t i = 0; i < numSubresources; ++i)
        {
            TransitionBarrier(resource, stateAfter, firstSubresource + i);
        }
    }
    else
    {
        TransitionBarrier(resource, stateAfter);
    }

    m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIndex, descriptorOffset, 1, resource.GetShaderResourceView(srv));

    TrackResource(resource);
}

void CommandList::SetUnorderedAccessView(
    uint32_t rootParameterIndex,
    uint32_t descrptorOffset,
    const Resource& resource,
    D3D12_RESOURCE_STATES stateAfter/* = D3D12_RESOURCE_STATE_UNORDERED_ACCESS*/,
    UINT firstSubresource/* = 0*/,
    UINT numSubresources/* = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/,
    const D3D12_UNORDERED_ACCESS_VIEW_DESC* uav/* = nullptr*/
)
{
    if (numSubresources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
    {
        for (uint32_t i = 0; i < numSubresources; ++i)
        {
            TransitionBarrier(resource, stateAfter, firstSubresource + i);
        }
    }
    else
    {
        TransitionBarrier(resource, stateAfter);
    }

    m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIndex, descrptorOffset, 1, resource.GetUnorderedAccessView(uav));

    TrackResource(resource);
}

void CommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex, uint32_t startInstance)
{
    FlushResourceBarriers();

    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        m_DynamicDescriptorHeap[i]->CommitStagedDescriptorsForDraw(*this);
    }

    m_d3d12CommandList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
}

void CommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex, int32_t baseVertex, uint32_t startInstance)
{
    FlushResourceBarriers();

    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        m_DynamicDescriptorHeap[i]->CommitStagedDescriptorsForDraw(*this);
    }

    m_d3d12CommandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, baseVertex, startInstance);
}

void CommandList::LoadTextureFromFile(Texture& texture, const std::wstring& fileName, TextureUsage textureUsage/* = TextureUsage::Albedo*/)
{
    fs::path filePath(fileName);
    if (!fs::exists(filePath))
    {
        throw std::exception("File not found.");
    }

    std::lock_guard<std::mutex> lock(ms_TextureCacheMutex);
    auto iter = ms_TextureCache.find(fileName);
    if (iter != ms_TextureCache.end())
    {
        texture.SetTextureUsage(textureUsage);
        texture.SetD3D12Resource(iter->second);
        texture.CreateViews();
        texture.SetName(fileName);
    }
    else
    {
        TexMetadata metadata;
        ScratchImage scratchImage;

        if (filePath.extension() == ".dds")
        {
            ThrowIfFailed(LoadFromDDSFile(
                fileName.c_str(),
                DDS_FLAGS_FORCE_RGB,
                &metadata,
                scratchImage));
        }
        else if (filePath.extension() == ".hdr")
        {
            ThrowIfFailed(LoadFromHDRFile(
                fileName.c_str(),
                &metadata,
                scratchImage));
        }
        else if (filePath.extension() == ".tga")
        {
            ThrowIfFailed(LoadFromTGAFile(
                fileName.c_str(),
                &metadata,
                scratchImage));
        }
        else
        {
            ThrowIfFailed(LoadFromWICFile(
                fileName.c_str(),
                WIC_FLAGS_FORCE_RGB,
                &metadata,
                scratchImage));
        }

        D3D12_RESOURCE_DESC textureDesc = {};
        switch (metadata.dimension)
        {
        case TEX_DIMENSION_TEXTURE1D:
            textureDesc = CD3DX12_RESOURCE_DESC::Tex1D(
                metadata.format,
                static_cast<UINT64>(metadata.width),
                static_cast<UINT16>(metadata.arraySize));
            break;
        case TEX_DIMENSION_TEXTURE2D:
            textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
                metadata.format,
                static_cast<UINT64>(metadata.width),
                static_cast<UINT>(metadata.height),
                static_cast<UINT16>(metadata.arraySize));
            break;
        case TEX_DIMENSION_TEXTURE3D:
            textureDesc = CD3DX12_RESOURCE_DESC::Tex3D(
                metadata.format,
                static_cast<UINT64>(metadata.width),
                static_cast<UINT>(metadata.height),
                static_cast<UINT16>(metadata.depth));
            break;
        default:
            throw std::exception("Invalid texture dimension.");
            break;
        }

        auto device = GetApp().GetDevice();
        Microsoft::WRL::ComPtr<ID3D12Resource> textureResource;

        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&textureResource)));

        texture.SetTextureUsage(textureUsage);
        texture.SetD3D12Resource(textureResource);
        texture.CreateViews();
        texture.SetName(fileName);

        // Update the global state tracker.
        ResourceStateTracker::AddGlobalResourceState(
            textureResource.Get(), D3D12_RESOURCE_STATE_COMMON);

        std::vector<D3D12_SUBRESOURCE_DATA> subresources(scratchImage.GetImageCount());
        const Image* pImages = scratchImage.GetImages();

        for (int i = 0; i < scratchImage.GetImageCount(); ++i)
        {
            auto& subresource = subresources[i];
            subresource.RowPitch = pImages[i].rowPitch;
            subresource.SlicePitch = pImages[i].slicePitch;
            subresource.pData = pImages[i].pixels;
        }

        CopyTextureSubresource(
            texture,
            0,
            static_cast<uint32_t>(subresources.size()),
            subresources.data());

        if (subresources.size() < textureResource->GetDesc().MipLevels)
        {
            GenerateMips(texture);
        }

        // Add the texture resource to the texture cache.
        ms_TextureCache[fileName] = textureResource.Get();
    }
}

void CommandList::CopyTextureSubresource(Texture& texture, uint32_t firstSubresource, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA* subresourceData)
{
    auto& destinationResource = texture.GetD3D12Resource();

    if (!destinationResource)
        return;

    auto& device = GetApp().GetDevice();

    // Resource must be in the copy-destination state.
    TransitionBarrier(texture, D3D12_RESOURCE_STATE_COPY_DEST);
    FlushResourceBarriers();

    UINT64 requiredSize = GetRequiredIntermediateSize(destinationResource.Get(), firstSubresource, numSubresources);

    // Create a temporary (intermediate) resource for uploading the subresources
    ComPtr<ID3D12Resource> intermediateResource;
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(requiredSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&intermediateResource)
    ));

    UpdateSubresources(m_d3d12CommandList.Get(), destinationResource.Get(), intermediateResource.Get(), 0, firstSubresource, numSubresources, subresourceData);

    TrackObject(intermediateResource);
    TrackObject(destinationResource);
}

bool CommandList::Close(CommandList& pendingCommandList)
{
    // Flush any remaining barriers.
    FlushResourceBarriers();

    m_d3d12CommandList->Close();

    // Flush pending resource barriers.
    uint32_t numPendingBarriers = m_ResourceStateTracker->FlushPendingResourceBarriers(pendingCommandList);

    // Commit the final resource state to the global state.
    m_ResourceStateTracker->CommitFinalResourceStates();

    return numPendingBarriers > 0;
}

void CommandList::Close()
{
    FlushResourceBarriers();
    m_d3d12CommandList->Close();
}

std::shared_ptr<CommandList> CommandList::GetGenerateMipsCommandList() const
{
    return m_ComputeCommandList;
}

void CommandList::Reset()
{
    ThrowIfFailed(m_d3d12CommandAllocator->Reset());
    ThrowIfFailed(m_d3d12CommandList->Reset(m_d3d12CommandAllocator.Get(), nullptr));

    m_ResourceStateTracker->Reset();
    m_UploadBuffer->Reset();

    ReleaseTrackedObjects();

    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        m_DynamicDescriptorHeap[i]->Reset();
        m_DescriptorHeaps[i] = nullptr;
    }

    m_RootSignature = nullptr;
    m_ComputeCommandList = nullptr;
}

void CommandList::Dispatch(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ)
{
    FlushResourceBarriers();

    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        m_DynamicDescriptorHeap[i]->CommitStagedDescriptorsForDispatch(*this);
    }

    m_d3d12CommandList->Dispatch(numGroupsX, numGroupsY, numGroupsZ);
}

void CommandList::GenerateMips(Texture& texture)
{
    if (m_d3d12CommandListType == D3D12_COMMAND_LIST_TYPE_COPY)
    {
        if (!m_ComputeCommandList)
        {
            m_ComputeCommandList = GetApp().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE)->GetCommandList();
        }
        m_ComputeCommandList->GenerateMips(texture);
        return;
    }

    auto& resource = texture.GetD3D12Resource();

    // If the texture doesn't have a valid resource, do nothing.
    if (!resource) return;
    auto resourceDesc = resource->GetDesc();

    // If the texture only has a single mip level (level 0)
    // do nothing.
    if (resourceDesc.MipLevels == 1) return;
    // Currently, only non-multi-sampled 2D textures are supported.
    if (resourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D ||
        resourceDesc.DepthOrArraySize != 1 ||
        resourceDesc.SampleDesc.Count > 1)
    {
        throw std::exception("GenerateMips is only supported for non-multi-sampled 2D Textures.");
    }

    ComPtr<ID3D12Resource> uavResource = resource;
    // Create an alias of the original resource.
    // This is done to perform a GPU copy of resources with different formats.
    // BGR -> RGB texture copies will fail GPU validation unless performed 
    // through an alias of the BRG resource in a placed heap.
    ComPtr<ID3D12Resource> aliasResource;

    // If the passed-in resource does not allow for UAV access
    // then create a staging resource that is used to generate
    // the mipmap chain.
    if (!texture.CheckUAVSupport() ||
        (resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0)
    {
        auto& device = GetApp().GetDevice();

        // Describe an alias resource that is used to copy the original texture.
        auto aliasDesc = resourceDesc;
        // Placed resources can't be render targets or depth-stencil views.
        aliasDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        aliasDesc.Flags &= ~(D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

        // Describe a UAV compatible resource that is used to perform
        // mipmapping of the original texture.
        auto uavDesc = aliasDesc;   // The flags for the UAV description must match that of the alias description.
        uavDesc.Format = Texture::GetUAVCompatableFormat(resourceDesc.Format);

        D3D12_RESOURCE_DESC resourceDescs[] = {
            aliasDesc,
            uavDesc
        };

        // Create a heap that is large enough to store a copy of the original resource.
        auto allocationInfo = device->GetResourceAllocationInfo(0, _countof(resourceDescs), resourceDescs);

        D3D12_HEAP_DESC heapDesc = {};
        heapDesc.SizeInBytes = allocationInfo.SizeInBytes;
        heapDesc.Alignment = allocationInfo.Alignment;
        heapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
        heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;

        ComPtr<ID3D12Heap> heap;
        ThrowIfFailed(device->CreateHeap(&heapDesc, IID_PPV_ARGS(&heap)));

        // Make sure the heap does not go out of scope until the command list
        // is finished executing on the command queue.
        TrackObject(heap);

        // Create a placed resource that matches the description of the 
        // original resource. This resource is used to copy the original 
        // texture to the UAV compatible resource.
        ThrowIfFailed(device->CreatePlacedResource(
            heap.Get(),
            0,
            &aliasDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&aliasResource)
        ));

        ResourceStateTracker::AddGlobalResourceState(aliasResource.Get(), D3D12_RESOURCE_STATE_COMMON);
        // Ensure the scope of the alias resource.
        TrackObject(aliasResource);

        // Create a UAV compatible resource in the same heap as the alias
        // resource.
        ThrowIfFailed(device->CreatePlacedResource(
            heap.Get(),
            0,
            &uavDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&uavResource)
        ));

        ResourceStateTracker::AddGlobalResourceState(uavResource.Get(), D3D12_RESOURCE_STATE_COMMON);
        // Ensure the scope of the UAV compatible resource.
        TrackObject(uavResource);

        // Add an aliasing barrier for the alias resource.
        AliasingBarrier(nullptr, aliasResource);

        // Copy the original resource to the alias resource.
        // This ensures GPU validation.
        CopyResource(aliasResource, resource);


        // Add an aliasing barrier for the UAV compatible resource.
        AliasingBarrier(aliasResource, uavResource);
    }

    // Generate mips with the UAV compatible resource.
    GenerateMips_UAV(Texture(uavResource, texture.GetTextureUsage()), Texture::IsSRGBFormat(resourceDesc.Format));

    if (aliasResource)
    {
        AliasingBarrier(uavResource, aliasResource);
        // Copy the alias resource back to the original resource.
        CopyResource(resource, aliasResource);
    }
}

void CommandList::GenerateMips_UAV(Texture& texture, bool isSRGB)
{
    if (!m_GenerateMipsPSO)
    {
        m_GenerateMipsPSO = std::make_unique<GenerateMipsPSO>();
    }

    m_d3d12CommandList->SetPipelineState(m_GenerateMipsPSO->GetPipelineState().Get());
    SetComputeRootSignature(m_GenerateMipsPSO->GetRootSignature());

    GenerateMipsCB generateMipsCB;
    generateMipsCB.IsSRGB = isSRGB;

    auto& resource = texture.GetD3D12Resource();
    auto resourceDesc = resource->GetDesc();

    // Create an SRV that uses the format of the original texture.
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = isSRGB ? Texture::GetSRGBFormat(resourceDesc.Format) : resourceDesc.Format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;  // Only 2D textures are supported (this was checked in the calling function).
    srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;

    for (uint32_t srcMip = 0; srcMip < resourceDesc.MipLevels - 1u; )
    {
        uint64_t srcWidth = resourceDesc.Width >> srcMip;
        uint32_t srcHeight = resourceDesc.Height >> srcMip;
        uint32_t dstWidth = static_cast<uint32_t>(srcWidth >> 1);
        uint32_t dstHeight = srcHeight >> 1;

        // 0b00(0): Both width and height are even.
        // 0b01(1): Width is odd, height is even.
        // 0b10(2): Width is even, height is odd.
        // 0b11(3): Both width and height are odd.
        generateMipsCB.SrcDimension = (srcHeight & 1) << 1 | (srcWidth & 1);

        // How many mipmap levels to compute this pass (max 4 mips per pass)
        DWORD mipCount;

        // The number of times we can half the size of the texture and get
        // exactly a 50% reduction in size.
        // A 1 bit in the width or height indicates an odd dimension.
        // The case where either the width or the height is exactly 1 is handled
        // as a special case (as the dimension does not require reduction).
        _BitScanForward(&mipCount, (dstWidth == 1 ? dstHeight : dstWidth) |
            (dstHeight == 1 ? dstWidth : dstHeight));

        // Maximum number of mips to generate is 4.
        mipCount = std::min<DWORD>(4, mipCount + 1);
        // Clamp to total number of mips left over.
        mipCount = (srcMip + mipCount) >= resourceDesc.MipLevels ?
            resourceDesc.MipLevels - srcMip - 1 : mipCount;

        // Dimensions should not reduce to 0.
        // This can happen if the width and height are not the same.
        dstWidth = std::max<DWORD>(1, dstWidth);
        dstHeight = std::max<DWORD>(1, dstHeight);

        generateMipsCB.SrcMipLevel = srcMip;
        generateMipsCB.NumMipLevels = mipCount;
        generateMipsCB.TexelSize.x = 1.0f / (float)dstWidth;
        generateMipsCB.TexelSize.y = 1.0f / (float)dstHeight;

        SetCompute32BitConstants(GenerateMips::GenerateMipsCB, generateMipsCB);

        SetShaderResourceView(GenerateMips::SrcMip, 0, texture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, srcMip, 1, &srvDesc);

        for (uint32_t mip = 0; mip < mipCount; ++mip)
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = resourceDesc.Format;
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture2D.MipSlice = srcMip + mip + 1;

            SetUnorderedAccessView(GenerateMips::OutMip, mip, texture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, srcMip + mip + 1, 1, &uavDesc);
        }

        // Pad any unused mip levels with a default UAV. Doing this keeps the DX12 runtime happy.
        if (mipCount < 4)
        {
            m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(GenerateMips::OutMip, mipCount, 4 - mipCount, m_GenerateMipsPSO->GetDefaultUAV());
        }

        Dispatch(Math::DivideByMultiple(dstWidth, 8), Math::DivideByMultiple(dstHeight, 8));

        UAVBarrier(texture);

        srcMip += mipCount;
    }
}

void CommandList::SetGraphicsRootSignature(const RootSignature& rootSignature)
{
    auto d3d12RootSignature = rootSignature.GetRootSignature().Get();
    if (m_RootSignature != d3d12RootSignature)
    {
        m_RootSignature = d3d12RootSignature;

        for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
        {
            m_DynamicDescriptorHeap[i]->ParseRootSignature(rootSignature);
        }

        m_d3d12CommandList->SetGraphicsRootSignature(m_RootSignature);

        TrackObject(m_RootSignature);
    }
}

void CommandList::SetComputeRootSignature(const RootSignature& rootSignature)
{
    auto d3d12RootSignature = rootSignature.GetRootSignature().Get();
    if (m_RootSignature != d3d12RootSignature)
    {
        m_RootSignature = d3d12RootSignature;

        for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
        {
            m_DynamicDescriptorHeap[i]->ParseRootSignature(rootSignature);
        }

        m_d3d12CommandList->SetComputeRootSignature(m_RootSignature);

        TrackObject(m_RootSignature);
    }
}

void CommandList::SetGraphics32BitConstants(uint32_t rootParameterIndex, uint32_t numConstants, const void* constants)
{
    m_d3d12CommandList->SetGraphicsRoot32BitConstants(rootParameterIndex, numConstants, constants, 0);
}

void CommandList::SetCompute32BitConstants(uint32_t rootParameterIndex, uint32_t numConstants, const void* constants)
{
    m_d3d12CommandList->SetComputeRoot32BitConstants(rootParameterIndex, numConstants, constants, 0);
}

