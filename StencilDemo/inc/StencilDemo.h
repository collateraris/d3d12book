#pragma once

#include <Camera.h>
#include <Game.h>
#include <Window.h>
#include <Mesh.h>
#include <Texture.h>
#include <RenderTarget.h>
#include <RootSignature.h>
#include <Scene.h>

#include <DirectXMath.h>

#include <StencilDemoUtils.h>

#include <unordered_map>

namespace dx12demo
{
    enum class EMeshes
    {
        rooms,
        skull
    };

    enum class EMaterial
    {
        bricks,
        checkertile,
        icemirror,
        skullMat,
        shadowMat,
    };

    enum class ETexture
    {
        bricksTex,
        checkboardTex,
        iceTex,
        white1x1Tex,
    };

    enum class ERenderLayer
    {
        Opaque = 0,
        Mirrors,
        Reflected,
        Transparent,
        Shadow,
    };

	class StencilDemo : public core::Game
	{
	public:

        using super = Game;

        StencilDemo(const std::wstring& name, int width, int height, bool vSync = false);

        virtual ~StencilDemo();
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

        void DrawRenderItem(core::CommandList& commandList, 
            const std::vector<stdu::RenderItem>& items, const stdu::Mat& matricesWithWorldIndentity);

    private:
        
        DirectX::XMMATRIX m_ModelMatrix;
        DirectX::XMMATRIX m_ViewMatrix;
        DirectX::XMMATRIX m_ProjectionMatrix;

        D3D12_RECT m_ScissorRect;

        stdu::DirLight m_DirLight;
        stdu::DirLight m_ReflectionDirLight;

        stdu::RenderPassData m_PassData;

        DirectX::XMVECTOR m_MirrorPlane = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // xy plane

        std::unordered_map<ETexture, core::Texture> m_Textures;
        std::unordered_map<EMaterial, stdu::MaterialConstant> m_Materials;
        std::unordered_map<EMeshes, std::unique_ptr<core::Mesh>> m_Meshes;
        std::unordered_map<ERenderLayer, std::vector<stdu::RenderItem>> m_RenderItems;
        
        core::RenderTarget m_RenderTarget;
        core::RootSignature m_SceneRootSignature;
        core::RootSignature m_QuadRootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_OpaqueScenePipelineState;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_TransparentScenePipelineState;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_MarkStencilMirrorsScenePipelineState;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_DrawStencilReflectionsScenePipelineState;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_ShadowScenePipelineState;
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

        float m_FoV;
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