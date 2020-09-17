#pragma once

#include <URootObject.h>
#include "TextureUsage.h"

#include <d3d12.h>
#include <wrl.h>

#include <map> // for std::map
#include <memory> // for std::unique_ptr
#include <mutex> // for std::mutex
#include <vector> // for std::vector

namespace dx12demo::core
{
    class Buffer;
    class ByteAddressBuffer;
    class ConstantBuffer;
    class DynamicDescriptorHeap;
    class GenerateMipsPSO;
    class IndexBuffer;
    class PanoToCubemapPSO;
    class RenderTarget;
    class Resource;
    class ResourceStateTracker;
    class StructuredBuffer;
    class RootSignature;
    class Texture;
    class UploadBuffer;
    class VertexBuffer;

    class CommandList : public URootObject
    {
    public:
        CommandList(D3D12_COMMAND_LIST_TYPE type);
        virtual ~CommandList();

        const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2>& GetGraphicsCommandList() const;

        /**
         * Set the currently bound descriptor heap.
         * Should only be called by the DynamicDescriptorHeap class.
         */
        void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* heap);

        /**
         * Transition a resource to a particular state.
         *
         * @param resource The resource to transition.
         * @param stateAfter The state to transition the resource to. The before state is resolved by the resource state tracker.
         * @param subresource The subresource to transition. By default, this is D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES which indicates that all subresources are transitioned to the same state.
         * @param flushBarriers Force flush any barriers. Resource barriers need to be flushed before a command (draw, dispatch, or copy) that expects the resource to be in a particular state can run.
         */
        void TransitionBarrier(const Resource& resource, D3D12_RESOURCE_STATES stateAfter, UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool flushBarriers = false);
    
        /**
         * Add a UAV barrier to ensure that any writes to a resource have completed
         * before reading from the resource.
         *
         * @param resource The resource to add a UAV barrier for.
         * @param flushBarriers Force flush any barriers. Resource barriers need to be
         * flushed before a command (draw, dispatch, or copy) that expects the resource
         * to be in a particular state can run.
         */
        void UAVBarrier(const Resource& resource, bool flushBarriers = false);

        /**
         * Add an aliasing barrier to indicate a transition between usages of two
         * different resources that occupy the same space in a heap.
         *
         * @param beforeResource The resource that currently occupies the heap.
         * @param afterResource The resource that will occupy the space in the heap.
         */
        void AliasingBarrier(const Resource& beforeResource, const Resource& afterResource, bool flushBarriers = false);

        void FlushResourceBarriers();

        void CopyResource(Resource& dstRes, const Resource& srcRes);

        void ReleaseTrackedObjects();

        /**
         * Set a dynamic constant buffer data to an inline descriptor in the root
         * signature.
         */
        void SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, size_t sizeInBytes, const void* bufferData);
        template<typename T>
        void SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, const T& data)
        {
            SetGraphicsDynamicConstantBuffer(rootParameterIndex, sizeof(T), &data);
        }

        /**
         * Set the SRV on the graphics pipeline.
         */
        void SetShaderResourceView(
            uint32_t rootParameterIndex,
            uint32_t descriptorOffset,
            const Resource& resource,
            D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            UINT firstSubresource = 0,
            UINT numSubresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            const D3D12_SHADER_RESOURCE_VIEW_DESC* srv = nullptr
        );

        /**
         * Set the UAV on the graphics pipeline.
         */
        void SetUnorderedAccessView(
            uint32_t rootParameterIndex,
            uint32_t descrptorOffset,
            const Resource& resource,
            D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            UINT firstSubresource = 0,
            UINT numSubresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            const D3D12_UNORDERED_ACCESS_VIEW_DESC* uav = nullptr
        );

        /**
         * Generate mips for the texture.
         * The first subresource is used to generate the mip chain.
         * Mips are automatically generated for textures loaded from files.
         */
        void GenerateMips(Texture& texture);

        void LoadTextureFromFile(Texture& texture, const std::wstring& fileName, TextureUsage textureUsage = TextureUsage::Albedo);
        void CopyTextureSubresource(Texture& texture, uint32_t firstSubresource, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA* subresourceData);
        /**
         * Draw geometry.
         */
        void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t startVertex = 0, uint32_t startInstance = 0);
        void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t startIndex = 0, int32_t baseVertex = 0, uint32_t startInstance = 0);

    protected:

        void TrackObject(const Microsoft::WRL::ComPtr<ID3D12Object>& object);

        void TrackResource(const Resource& res);

    private:

        // Resource state tracker is used by the command list to track (per command list)
        // the current state of a resource. The resource state tracker also tracks the 
        // global state of a resource in order to minimize resource state transitions.
        std::unique_ptr<ResourceStateTracker> m_ResourceStateTracker;

        // Resource created in an upload heap. Useful for drawing of dynamic geometry
        // or for uploading constant buffer data that changes every draw call.
        std::unique_ptr<UploadBuffer> m_UploadBuffer;

        // The dynamic descriptor heap allows for descriptors to be staged before
        // being committed to the command list. Dynamic descriptors need to be
        // committed before a Draw or Dispatch.
        std::unique_ptr<DynamicDescriptorHeap> m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

        D3D12_COMMAND_LIST_TYPE m_d3d12CommandListType;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> m_d3d12CommandList;

        using TrackedObjects = std::vector < Microsoft::WRL::ComPtr<ID3D12Object> >;

        TrackedObjects m_TrackedObjects;

        // Keep track of loaded textures to avoid loading the same texture multiple times.
        static std::map<std::wstring, ID3D12Resource* > ms_TextureCache;
        static std::mutex ms_TextureCacheMutex;
    
    };
}