#pragma once

#include <Camera.h>
#include <CameraEuler.h>
#include <Game.h>
#include <Window.h>
#include <Mesh.h>
#include <Texture.h>
#include <RenderTarget.h>
#include <RootSignature.h>
#include <Scene.h>
#include <Terrain.h>
#include <Frustum.h>

#include <EnvironmentMapRenderPass.h>

#include <DirectXMath.h>

namespace dx12demo
{
	class Terrain_Project : public core::Game
	{
        struct LightBuffer
        {
            DirectX::XMFLOAT4 ambientColor;
            DirectX::XMFLOAT4 diffuseColor;
            DirectX::XMFLOAT3 lightDirection;
        };

	public:

        using super = Game;

        Terrain_Project(const std::wstring& name, int width, int height, bool vSync = false);

        virtual ~Terrain_Project();
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

        virtual void OnKeyReleased(core::KeyEventArgs& e) override;
        /**
         * Invoked when the mouse wheel is scrolled while the registered window has focus.
         */
        virtual void OnMouseMoved(core::MouseMotionEventArgs& e) override;
        virtual void OnMouseWheel(core::MouseWheelEventArgs& e) override;

        virtual void OnDPIScaleChanged(core::DPIScaleEventArgs& e) override;

        virtual void OnResize(core::ResizeEventArgs& e) override;

        void RescaleRenderTarget(float scale);

        void OnGUI();

    private:
        
        core::EnvironmentMapRenderPass m_envRenderPass;

        float m_FoV;

        DirectX::XMMATRIX m_ModelMatrix;
        DirectX::XMMATRIX m_ViewMatrix;
        DirectX::XMMATRIX m_ProjectionMatrix;

        D3D12_RECT m_ScissorRect;

        core::Texture m_TerrainTexture;

        core::Terrain m_Scene;
        //core::Scene m_Sponza;
        LightBuffer m_DirLight;
        // HDR Render target
        core::RenderTarget m_RenderTarget;
        core::RootSignature m_SceneRootSignature;
        core::RootSignature m_QuadRootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_ScenePipelineState;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_QuadPipelineState;

        int m_Width;
        int m_Height;
        float m_RenderScale;

        core::Camera m_Camera;
        //CameraEuler m_CameraEuler;
        struct alignas(16) CameraData
        {
            DirectX::XMVECTOR m_InitialCamPos;
            DirectX::XMVECTOR m_InitialCamRot;
            float m_InitialFov;
        };
        CameraData* m_pAlignedCameraData;

        core::Frustum m_Frustum;

        // Camera controller
        float m_Forward;
        float m_Backward;
        float m_Left;
        float m_Right;
        float m_Up;
        float m_Down;

        float m_Pitch;
        float m_Yaw;

        // Rotate the lights in a circle.
        bool m_AnimateLights;
        // Set to true if the Shift key is pressed.
        bool m_Shift;

        const float m_CameraStep = 1.f;

        double m_FPS = 0.;

        bool m_ContentLoaded;
	};
}