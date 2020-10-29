#pragma once

#include <Camera.h>
#include <Game.h>
#include <Window.h>
#include <Mesh.h>
#include <Texture.h>
#include <RenderTarget.h>
#include <RootSignature.h>
#include <Scene.h>
#include <Terrain.h>
#include <Frustum.h>
#include <Light.h>
#include <config_sys/Config.h>
#include <GridViewFrustums.h>
#include <LightCulling.h>
#include <DepthBufferRenderPass.h>
#include <DebugDepthBufferRenderPass.h>
#include <ForwardPlusRenderPass.h>
#include <QuadRenderPass.h>

#include <EnvironmentMapRenderPass.h>

#include <DirectXMath.h>

namespace dx12demo
{
	class ForwardPlusDemo : public core::Game
	{
        struct LightBuffer
        {
            DirectX::XMVECTOR posWS;
            DirectX::XMVECTOR dirWS;
        };

	public:

        using super = Game;

        ForwardPlusDemo(const std::wstring& name, int width, int height, bool vSync = false);

        virtual ~ForwardPlusDemo();
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

        std::unique_ptr<core::Config> m_Config;
        
        core::EnvironmentMapRenderPass m_envRenderPass;
        core::DepthBufferRenderPass m_DepthBufferRenderPass;
        core::DebugDepthBufferRenderPass m_DebugDepthBufferRenderPass;
        core::ForwardPlusRenderPass m_ForwardPlusRenderPass;
        core::QuadRenderPass m_QuadRenderPass;

        std::vector<core::Light> m_Lights;
        std::vector<LightBuffer> m_LightsBufferData;

        float m_FoV;

        DirectX::XMMATRIX m_ModelMatrix;
        DirectX::XMMATRIX m_ViewMatrix;
        DirectX::XMMATRIX m_ProjectionMatrix;

        D3D12_RECT m_ScissorRect;

        core::Scene m_Sponza;
        // HDR Render target
        core::RenderTarget m_RenderTarget;
        core::RootSignature m_SceneRootSignature;
        core::RootSignature m_QuadRootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_ScenePipelineState;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_QuadPipelineState;

        // For the light index list, we need to make a guess as to the average 
        // number of overlapping lights per tile. It could be possible to refine this
        // value at runtime (if it is underestimated) but for now, I'll just take a guess
        // of about 200 lights (which may be an overestimation, but better over than under). 
        // The total size of the buffer will be determined by the grid size but for 16x16
        // tiles at 1080p, we would need 120x68 tiles * 200 light indices * 4 bytes (to store a uint)
        // making the light index list 6,528,000 bytes (6.528 MB)
        const uint32_t AVERAGE_OVERLAPPING_LIGHTS_PER_TILE = 200u;
        uint16_t m_LightCullingBlockSize = 16;
        core::GridViewFrustum m_ComputeGridFrustums;
        core::LightCulling m_ComputeLightCulling;
        core::DispatchParams m_CSDispatchParams;
        core::ScreenToViewParams m_ScreenToViewParams;

        std::function<void(std::shared_ptr<core::CommandList>&, std::shared_ptr<core::Material>&)> m_MaterialDrawFun;
        std::function<void(std::shared_ptr<core::CommandList>&, std::shared_ptr<core::Material>&)> m_ForwardPlusDrawFun;
        std::function<void(std::shared_ptr<core::CommandList>&, std::shared_ptr<core::Material>&)> m_DepthBufferDrawFun;

        int m_Width;
        int m_Height;
        float m_RenderScale;

        core::Camera m_Camera;
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

        const float m_CameraStep = 5.f;

        double m_FPS = 0.;

        bool m_ContentLoaded;
	};
}