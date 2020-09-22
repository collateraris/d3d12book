#pragma once

#include "DescriptorAllocation.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include <memory>
#include <unordered_map>
#include <string>

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

namespace dx12demo::core
{
	class Window;
	class Game;
	class CommandQueue;
	class DescriptorAllocator;

	class Application
	{
		using WindowPtr = std::shared_ptr<Window>;
		using WindowMap = std::unordered_map< HWND, WindowPtr >;
		using WindowNameMap = std::unordered_map< std::wstring, WindowPtr >;
	public:

        static Application& Create(HINSTANCE hInst);

        static void Destroy();

		bool IsTearingSupported() const;

		/**
		 * Check if the requested multisample quality is supported for the given format.
		 */
		DXGI_SAMPLE_DESC GetMultisampleQualityLevels(DXGI_FORMAT format, UINT numSamples, D3D12_MULTISAMPLE_QUALITY_LEVEL_FLAGS flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE) const;

		std::shared_ptr<Window> CreateRenderWindow(const std::wstring& windowName, int clientWidth, int clientHeight, bool vSync = true);
		void DestroyWindow(const std::wstring& windowName);
		void DestroyWindow(std::shared_ptr<Window> window);
		std::shared_ptr<Window> GetWindowByName(const std::wstring& windowName);
		void GetWindowByhWnd(HWND hWnd, WindowPtr& pWindow);

		int Run(std::shared_ptr<Game> pGame);
		void Quit(int exitCode = 0);

		const Microsoft::WRL::ComPtr<ID3D12Device2>& GetDevice() const;
		std::shared_ptr<CommandQueue> GetCommandQueue(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT) const;
		void Flush();

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type);
		UINT GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const;

		DescriptorAllocation AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors = 1);

		void ReleaseStaleDescriptors(uint64_t finishedFrame);

		uint64_t GetFrameCount() const;

	protected:

		// Create an application instance.
		Application(HINSTANCE hInst);
		// Destroy the application instance and all windows associated with this application.
		virtual ~Application();

		// Initialize the application instance.
		void Initialize();

		friend LRESULT CALLBACK ::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

		Microsoft::WRL::ComPtr<IDXGIAdapter4> GetAdapter(bool bUseWarp);
		Microsoft::WRL::ComPtr<ID3D12Device2> CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter);
		bool CheckTearingSupport();

		void RemoveWindow(HWND hWnd);

		bool IsWindowsEmpty() const;

		void IncFrameCount();

	private:

		Application() = delete;
		Application(const Application& copy) = delete;
		Application& operator=(const Application& other) = delete;

		// The application instance handle that this application was created with.
		HINSTANCE m_hInstance;

		Microsoft::WRL::ComPtr<IDXGIAdapter4> m_dxgiAdapter;
		Microsoft::WRL::ComPtr<ID3D12Device2> m_d3d12Device;

		std::shared_ptr<CommandQueue> m_DirectCommandQueue;
		std::shared_ptr<CommandQueue> m_ComputeCommandQueue;
		std::shared_ptr<CommandQueue> m_CopyCommandQueue;

		std::unique_ptr<DescriptorAllocator> m_DescriptorAllocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

		bool m_TearingSupported;

		WindowMap m_Windows;
		WindowNameMap m_WindowByName;

		uint64_t m_FrameCount = 0;
	};
}