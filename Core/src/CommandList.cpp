#include <CommandList.h>

#include <DX12LibPCH.h>
#include <Application.h>
#include <Resource.h>
#include <ResourceStateTracker.h>
#include <UploadBuffer.h>
#include <DynamicDescriptorHeap.h>
#include <Texture.h>

using namespace dx12demo::core;

void CommandList::TransitionBarrier(const Resource& resource, D3D12_RESOURCE_STATES stateAfter, UINT subResource, bool flushBarriers)
{
    auto d3d12Resource = resource.GetD3D12Resource();
    if (d3d12Resource)
    {
        // The "before" state is not important. It will be resolved by the resource state tracker.
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(d3d12Resource.Get(), D3D12_RESOURCE_STATE_COMMON, stateAfter, subResource);
        m_ResourceStateTracker->ResourceBarrier(barrier);
    }

    if (flushBarriers)
    {
        FlushResourceBarriers();
    }
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
    auto d3d12BeforeResource = beforeResource.GetD3D12Resource();
    auto d3d12AfterResource = afterResource.GetD3D12Resource();
    auto barrier = CD3DX12_RESOURCE_BARRIER::Aliasing(d3d12BeforeResource.Get(), d3d12AfterResource.Get());

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
    TransitionBarrier(dstRes, D3D12_RESOURCE_STATE_COPY_DEST);
    TransitionBarrier(srcRes, D3D12_RESOURCE_STATE_COPY_SOURCE);

    FlushResourceBarriers();

    m_d3d12CommandList->CopyResource(dstRes.GetD3D12Resource().Get(), srcRes.GetD3D12Resource().Get());

    TrackResource(dstRes);
    TrackResource(srcRes);
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

