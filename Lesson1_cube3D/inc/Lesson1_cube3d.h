#pragma once

#include <Game.h>
#include <Window.h>
#include <Mesh.h>
#include <Texture.h>
#include <RenderTarget.h>
#include <RootSignature.h>

#include <DirectXMath.h>

namespace dx12demo
{
	class Lesson1_cube3d : public core::Game
	{
	public:

        using super = Game;

        Lesson1_cube3d(const std::wstring& name, int width, int height, bool vSync = false);
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

        virtual void OnDPIScaleChanged(core::DPIScaleEventArgs& e) override;

        virtual void OnResize(core::ResizeEventArgs& e) override;

    private:
        
        float m_FoV;

        DirectX::XMMATRIX m_ModelMatrix;
        DirectX::XMMATRIX m_ViewMatrix;
        DirectX::XMMATRIX m_ProjectionMatrix;

        D3D12_RECT m_ScissorRect;

        std::unique_ptr<core::Mesh> m_SphereMesh;
        core::Texture m_EarthTexture;
        // HDR Render target
        core::RenderTarget m_RenderTarget;
        core::RootSignature m_RootSignature;
        core::RootSignature m_QuadRootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_QuadPipelineState;

        int m_Width;
        int m_Height;

        bool m_ContentLoaded;
	};
}