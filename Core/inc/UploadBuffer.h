#pragma once

#include <URootObject.h>
#include <Defines.h>

#include <wrl.h>
#include <d3d12.h>

#include <memory>
#include <deque>

namespace dx12demo::core
{
	class UploadBuffer : public URootObject
	{
    public:
        // Use to upload data to the GPU
        struct Allocation
        {
            void* CPU = nullptr;
            D3D12_GPU_VIRTUAL_ADDRESS GPU;
        };

        explicit UploadBuffer(size_t pageSize = _2MB);

        virtual ~UploadBuffer();

        size_t GetPageSize() const;

        Allocation Allocate(size_t sizeInBytes, size_t alignment);

        void Reset();

    private:

        struct Page : URootObject
        {
            Page(size_t sizeInBytes);
            virtual ~Page();

            bool HasSpace(size_t sizeInBytes, size_t alignment) const;

            Allocation Allocate(size_t sizeInBytes, size_t alignment);

            // Reset the page for reuse.
            void Reset();

        private:

            Microsoft::WRL::ComPtr<ID3D12Resource> m_d3d12Resource;

            // Base pointer.
            void* m_CPUPtr = nullptr;
            D3D12_GPU_VIRTUAL_ADDRESS m_GPUPtr;

            // Allocated page size.
            size_t m_PageSize;
            // Current allocation offset in bytes.
            size_t m_Offset;
        };

        // A pool of memory pages.
        using PagePool = std::deque< std::shared_ptr<Page> >;

        // Request a page from the pool of available pages
        // or create a new page if there are no available pages.
        std::shared_ptr<Page> RequestPage();

        PagePool m_PagePool;
        PagePool m_AvailablePages;

        std::shared_ptr<Page> m_CurrentPage;

        // The size of each page of memory.
        size_t m_PageSize;
	};
}