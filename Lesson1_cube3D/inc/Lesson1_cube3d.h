#pragma once

#include <Game.h>
#include <Window.h>

#include <DirectXMath.h>

namespace dx12demo
{
    namespace core
    {
        class Application;
    }

	class Lesson1_cube3d : public core::Game
	{
	public:

        using super = Game;

        Lesson1_cube3d(core::Application* app, const std::wstring& name, int width, int height, bool vSync = false);
        /**
         *  Load content required for the demo.
         */
        virtual bool LoadContent() override;

        /**
         *  Unload demo specific content that was loaded in LoadContent.
         */
        virtual void UnloadContent() override;
    protected:
        /**
         *  Update the game logic.
         */
        virtual void OnUpdate(core::UpdateEventArgs& e) override;

        /**
         *  Render stuff.
         */
        virtual void OnRender(core::RenderEventArgs& e) override;

        /**
         * Invoked by the registered window when a key is pressed
         * while the window has focus.
         */
        virtual void OnKeyPressed(core::KeyEventArgs& e) override;

        /**
         * Invoked when the mouse wheel is scrolled while the registered window has focus.
         */
        virtual void OnMouseWheel(core::MouseWheelEventArgs& e) override;


        virtual void OnResize(core::ResizeEventArgs& e) override;

    private:
        // Helper functions
        // Transition a resource
        void TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
            Microsoft::WRL::ComPtr<ID3D12Resource> resource,
            D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);

        // Clear a render target view.
        void ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
            D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor);

        // Clear the depth of a depth-stencil view.
        void ClearDepth(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
            D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth = 1.0f);

        // Create a GPU buffer.
        void UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
            ID3D12Resource** pDestinationResource, ID3D12Resource** pIntermediateResource,
            size_t numElements, size_t elementSize, const void* bufferData,
            D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

        // Resize the depth buffer to match the size of the client area.
        void ResizeDepthBuffer(int width, int height);

        uint64_t m_FenceValues[core::Window::BufferCount] = {};

        // Vertex buffer for the cube.
        Microsoft::WRL::ComPtr<ID3D12Resource> m_VertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
        // Index buffer for the cube.
        Microsoft::WRL::ComPtr<ID3D12Resource> m_IndexBuffer;
        D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

        // Depth buffer.
        Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthBuffer;
        // Descriptor heap for depth buffer.
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DSVHeap;

        // Root signature
        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;

        // Pipeline state object.
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;

        D3D12_VIEWPORT m_Viewport;
        D3D12_RECT m_ScissorRect;

        float m_FoV;

        DirectX::XMMATRIX m_ModelMatrix;
        DirectX::XMMATRIX m_ViewMatrix;
        DirectX::XMMATRIX m_ProjectionMatrix;

        bool m_ContentLoaded;
	};
}