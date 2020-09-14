#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_5.h>
#include <memory>

#include <Events.h>
#include <Application.h>
#include <HighResolutionClock.h>

namespace dx12demo::core
{
	class Game;

	class Window
	{
	public:
		// Number of swapchain back buffers.
		static const UINT BufferCount = 3;

		HWND GetWindowHandle() const;

		void Destroy();

		const std::wstring& GetWindowName() const;

		int GetClientWidth() const;
		int GetClientHeight() const;

		bool IsVSync() const;
		void SetVSync(bool vSync);
		void ToggleVSync();

		bool IsFullScreen() const;

		// Set the fullscreen state of the window.
		void SetFullscreen(bool fullscreen);
		void ToggleFullscreen();

		void Show();
		void Hide();

		UINT GetCurrentBackBufferIndex() const;

		UINT Present();

		D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRenderTargetView() const;

		Microsoft::WRL::ComPtr<ID3D12Resource> GetCurrentBackBuffer() const;

	protected:
		// The Window procedure needs to call protected methods of this class.
		friend LRESULT CALLBACK ::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

		// Only the application can create a window.
		friend class Application;
		// The DirectXTemplate class needs to register itself with a window.
		friend class Game;

		Window() = delete;
		Window(Application* app, HWND hWnd, const std::wstring& windowName, int clientWidth, int clientHeight, bool vSync);
		virtual ~Window();

		// Register a Game with this window. This allows
		// the window to callback functions in the Game class.
		void RegisterCallbacks(std::shared_ptr<Game> pGame);

		// Update and Draw can only be called by the application.
		virtual void OnUpdate(UpdateEventArgs& e);
		virtual void OnRender(RenderEventArgs& e);

		// A keyboard key was pressed
		virtual void OnKeyPressed(KeyEventArgs& e);
		// A keyboard key was released
		virtual void OnKeyReleased(KeyEventArgs& e);

		// The mouse was moved
		virtual void OnMouseMoved(MouseMotionEventArgs& e);
		// A button on the mouse was pressed
		virtual void OnMouseButtonPressed(MouseButtonEventArgs& e);
		// A button on the mouse was released
		virtual void OnMouseButtonReleased(MouseButtonEventArgs& e);
		// The mouse wheel was moved.
		virtual void OnMouseWheel(MouseWheelEventArgs& e);

		// The window was resized.
		virtual void OnResize(ResizeEventArgs& e);

		// Create the swapchian.
		Microsoft::WRL::ComPtr<IDXGISwapChain4> CreateSwapChain();

		// Update the render target views for the swapchain back buffers.
		void UpdateRenderTargetViews();

	private:

		Window(const Window& copy) = delete;
		Window& operator=(const Window& other) = delete;

		HWND m_hWnd;
		std::wstring m_WindowName;

		int m_ClientWidth;
		int m_ClientHeight;
		bool m_VSync;
		bool m_Fullscreen;

		HighResolutionClock m_UpdateClock;
		HighResolutionClock m_RenderClock;
		uint64_t m_FrameCounter;

		std::weak_ptr<Game> m_pGame;
		Application* m_pApplication = nullptr;

		Microsoft::WRL::ComPtr<IDXGISwapChain4> m_dxgiSwapChain;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_d3d12RTVDescriptorHeap;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_d3d12BackBuffers[BufferCount];

		UINT m_RTVDescriptorSize;
		UINT m_CurrentBackBufferIndex;

		RECT m_WindowRect;
		bool m_IsTearingSupported;
	};
}