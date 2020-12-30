#include <Terrain_Project.h>

#include <Application.h>
#include <CommandQueue.h>
#include <DX12LibPCH.h>
#include <Helpers.h>
#include <Window.h>
#include <Material.h>

using namespace dx12demo;
using namespace DirectX;

const float SCREEN_DEPTH = 1000.0f;
const float SCREEN_NEAR = 0.1f;

enum class SceneRootParameters
{
    MatricesCB,         // ConstantBuffer<Mat> MatCB : register(b0);
    DirLight,           // StructuredBuffer<PointLight> PointLights : register( b1 );
    AmbientTex,         // Texture2D AmbientTexture : register( t0 );
    NumRootParameters
};

struct __declspec(align(16)) Mat
{
    XMMATRIX ModelMatrix;
    XMMATRIX ModelViewMatrix;
    XMMATRIX InverseTransposeModelViewMatrix;
    XMMATRIX ModelViewProjectionMatrix;
};

void XM_CALLCONV ComputeMatrices(const XMMATRIX& model, const CXMMATRIX& view, const CXMMATRIX& projection, Mat& mat)
{
    mat.ModelMatrix = model;
    mat.ModelViewMatrix = model * view;
    mat.InverseTransposeModelViewMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, mat.ModelViewMatrix));
    mat.ModelViewProjectionMatrix = mat.ModelViewMatrix * projection;
}


static void ShowHelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}


ForwardPlusDemo::ForwardPlusDemo(const std::wstring& name, int width, int height, bool vSync)
    : super(name, width, height, vSync)
    , m_ScissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX))
    , m_Forward(0)
    , m_Backward(0)
    , m_Left(0)
    , m_Right(0)
    , m_Up(0)
    , m_Down(0)
    , m_Pitch(0)
    , m_Yaw(0)
    , m_AnimateLights(false)
    , m_Shift(false)
    , m_Width(0)
    , m_Height(0)
    , m_RenderScale(1.0f)
{
    XMVECTOR cameraPos = XMVectorSet(50, 2, -7, 1);
    XMVECTOR cameraTarget = XMVectorSet(0, 5, 0, 1);
    XMVECTOR cameraUp = XMVectorSet(0, 1, 0, 0);

    m_Camera = std::make_shared<core::Camera>();

    m_Camera->set_LookAt(cameraPos, cameraTarget, cameraUp);
    m_Camera->set_Projection(45.0f, static_cast<float>(width) / height, SCREEN_NEAR, SCREEN_DEPTH);

    m_ViewMatrix = m_Camera->get_ViewMatrix();
    m_ProjectionMatrix = m_Camera->get_ProjectionMatrix();

    m_pAlignedCameraData = (CameraData*)_aligned_malloc(sizeof(CameraData), 16);

    m_pAlignedCameraData->m_InitialCamPos = m_Camera->get_Translation();
    m_pAlignedCameraData->m_InitialCamRot = m_Camera->get_Rotation();
    m_pAlignedCameraData->m_InitialFov = m_Camera->get_FoV();
    /*
    m_CameraEuler.set_LookAt(cameraPos, cameraTarget, cameraUp);
    m_CameraEuler.set_Projection(45.0f, static_cast<float>(width) / height, SCREEN_NEAR, SCREEN_DEPTH);

    m_ViewMatrix = m_CameraEuler.get_ViewMatrix();
    m_ProjectionMatrix = m_CameraEuler.get_ProjectionMatrix();
    */

    m_DirLight.ambientColor = { 0.05f, 0.05f, 0.05f, 1.0f };
    m_DirLight.diffuseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    m_DirLight.lightDirection = { -0.5f, -1.0f, 0.0f };
}

ForwardPlusDemo::~ForwardPlusDemo()
{
    _aligned_free(m_pAlignedCameraData);
}

bool ForwardPlusDemo::LoadContent()
{
    auto& app = GetApp();
    auto& device = app.GetDevice();
    auto commandQueue = app.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    auto commandList = commandQueue->GetCommandList();

    core::TerrainInfo terraInfo;
    terraInfo.heightMapPath = "Assets/Textures/heightmap01.bmp";
    m_Scene.Generate(*commandList, terraInfo);

    //m_Sponza.LoadFromFile(commandList, L"Assets/models/crytek-sponza/sponza_nobanner.obj", true);

    // Create an HDR intermediate render target.
    DXGI_FORMAT HDRFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

    // Create an off-screen render target with a single color buffer and a depth buffer.
    auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(HDRFormat, m_Width, m_Height);
    colorDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE colorClearValue;
    colorClearValue.Format = colorDesc.Format;
    colorClearValue.Color[0] = 0.4f;
    colorClearValue.Color[1] = 0.6f;
    colorClearValue.Color[2] = 0.9f;
    colorClearValue.Color[3] = 1.0f;

    core::Texture ScreenTexture = core::Texture(colorDesc, &colorClearValue,
        TextureUsage::RenderTarget,
        L"Screen Texture");

    // Create a depth buffer for the HDR render target.
    auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(depthBufferFormat, m_Width, m_Height);
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE depthClearValue;
    depthClearValue.Format = depthDesc.Format;
    depthClearValue.DepthStencil = { 1.0f, 0 };

    core::Texture depthTexture = core::Texture(depthDesc, &depthClearValue,
        TextureUsage::Depth,
        L"Depth Render Target");

    // Attach the HDR texture to the HDR render target.
    m_RenderTarget.AttachTexture(core::AttachmentPoint::Color0, ScreenTexture);
    m_RenderTarget.AttachTexture(core::AttachmentPoint::DepthStencil, depthTexture);

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    {
        m_skydoomRP = core::SkyDoomFabric::GetGradientType(m_Camera, m_RenderTarget.GetRenderTargetFormats(), depthBufferFormat, featureData.HighestVersion);
    }

    {
        core::PerturbedCloudsRenderPassInfo info;
        info.camera = m_Camera;
        info.rtvFormats = m_RenderTarget.GetRenderTargetFormats();
        info.depthBufFormat = depthBufferFormat;
        info.rootSignatureVersion = featureData.HighestVersion;
        m_perturbedCloudsRP.LoadContent(&info);
    }

    // Create a root signature for the forward shading (scene) pipeline.
    {
        commandList->LoadTextureFromFile(m_TerrainTexture, L"Assets/Textures/terrain1.dds");

        // Load the  shaders.
        ComPtr<ID3DBlob> vs;
        ComPtr<ID3DBlob> ps;
        ThrowIfFailed(D3DReadFileToBlob(L"ForwardVS.cso", &vs));
        ThrowIfFailed(D3DReadFileToBlob(L"ForwardPS.cso", &ps));

        // Allow input layout and deny unnecessary access to certain pipeline stages.
        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        CD3DX12_ROOT_PARAMETER1 rootParameters[static_cast<size_t>(SceneRootParameters::NumRootParameters)];
        rootParameters[static_cast<int>(SceneRootParameters::MatricesCB)].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);      
        rootParameters[static_cast<int>(SceneRootParameters::DirLight)].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
        CD3DX12_DESCRIPTOR_RANGE1 ambientTexDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
        rootParameters[static_cast<int>(SceneRootParameters::AmbientTex)].InitAsDescriptorTable(1, &ambientTexDescrRange, D3D12_SHADER_VISIBILITY_PIXEL);

        CD3DX12_STATIC_SAMPLER_DESC linearRepeatSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
        rootSignatureDescription.Init_1_1(static_cast<size_t>(SceneRootParameters::NumRootParameters), rootParameters, 1, &linearRepeatSampler, rootSignatureFlags);

        m_SceneRootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

        struct PipelineStateStream
        {
            CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
            CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
            CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
            CD3DX12_PIPELINE_STATE_STREAM_VS VS;
            CD3DX12_PIPELINE_STATE_STREAM_PS PS;
            CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
            CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
            CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DepthStencilState;
        } pipelineStateStream;

        pipelineStateStream.pRootSignature = m_SceneRootSignature.GetRootSignature().Get();
        pipelineStateStream.InputLayout = { core::PosNormTexVertex::InputElements, core::PosNormTexVertex::InputElementCount };
        pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vs.Get());
        pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
        pipelineStateStream.DSVFormat = m_RenderTarget.GetDepthStencilFormat();
        pipelineStateStream.RTVFormats = m_RenderTarget.GetRenderTargetFormats();
        pipelineStateStream.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

        D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
            sizeof(PipelineStateStream), &pipelineStateStream
        };
        ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_ScenePipelineState)));
    }

    {
        // Load the Quad shaders.

        ComPtr<ID3DBlob> vs;
        ComPtr<ID3DBlob> ps;
        ThrowIfFailed(D3DReadFileToBlob(L"ScreenQuadVS.cso", &vs));
        ThrowIfFailed(D3DReadFileToBlob(L"ScreenQuadPS.cso", &ps));

        CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

        CD3DX12_ROOT_PARAMETER1 rootParameters[1];
        rootParameters[0].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

        CD3DX12_STATIC_SAMPLER_DESC linearClampsSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
        rootSignatureDescription.Init_1_1(1, rootParameters, 1, &linearClampsSampler);

        m_QuadRootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

        CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
        rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

        struct PipelineStateStream
        {
            CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
            CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
            CD3DX12_PIPELINE_STATE_STREAM_VS VS;
            CD3DX12_PIPELINE_STATE_STREAM_PS PS;
            CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER Rasterizer;
            CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
        } pipelineStateStream;

        pipelineStateStream.pRootSignature = m_QuadRootSignature.GetRootSignature().Get();
        pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vs.Get());
        pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
        pipelineStateStream.Rasterizer = rasterizerDesc;
        pipelineStateStream.RTVFormats = m_pWindow->GetRenderTarget().GetRenderTargetFormats();

        D3D12_PIPELINE_STATE_STREAM_DESC PipelineStateStreamDesc = {
            sizeof(PipelineStateStream), &pipelineStateStream
        };
        ThrowIfFailed(device->CreatePipelineState(&PipelineStateStreamDesc, IID_PPV_ARGS(&m_QuadPipelineState)));
    }

    auto fenceValue = commandQueue->ExecuteCommandList(commandList);
    commandQueue->WaitForFenceValue(fenceValue);

    return true;
}

void ForwardPlusDemo::UnloadContent()
{
    m_ContentLoaded = false;
}


void ForwardPlusDemo::OnResize(core::ResizeEventArgs& e)
{
    super::OnResize(e);

    if (m_Width != e.Width || m_Height != e.Height)
    {
        m_Width = std::max(1, e.Width);
        m_Height = std::max(1, e.Height);

        float fov = m_Camera->get_FoV();
        float aspectRatio = m_Width / (float)m_Height;
        m_Camera->set_Projection(fov, aspectRatio, 0.1f, 100.0f);
        //m_CameraEuler.set_Projection(m_CameraEuler.get_FoV(), aspectRatio, SCREEN_NEAR, SCREEN_DEPTH);

        RescaleRenderTarget(m_RenderScale);
    }
}

void ForwardPlusDemo::OnUpdate(core::UpdateEventArgs& e)
{
    static uint64_t frameCount = 0;
    static double totalTime = 0.0;

    super::OnUpdate(e);

    totalTime += e.ElapsedTime;
    frameCount++;

    if (totalTime > 1.0)
    {
        m_FPS = frameCount / totalTime;

        char buffer[512];
        sprintf_s(buffer, "FPS: %f\n", m_FPS);
        OutputDebugStringA(buffer);

        frameCount = 0;
        totalTime = 0.0;
    }

    {
        //m_CameraEuler.Movement(e.ElapsedTime);
    }
    {
        // Update the camera.
        float speedMultipler = (m_Shift ? 16.0f : 4.0f);

        XMVECTOR cameraTranslate = XMVectorSet(m_Right - m_Left, 0.0f, m_Forward - m_Backward, 1.0f) * speedMultipler * static_cast<float>(e.ElapsedTime);
        XMVECTOR cameraPan = XMVectorSet(0.0f, m_Up - m_Down, 0.0f, 1.0f) * speedMultipler * static_cast<float>(e.ElapsedTime);
        m_Camera->Translate(cameraTranslate, core::ESpace::Local);
        m_Camera->Translate(cameraPan, core::ESpace::Local);

        XMVECTOR cameraRotation = XMQuaternionRotationRollPitchYaw(XMConvertToRadians(m_Pitch), XMConvertToRadians(m_Yaw), 0.0f);
        m_Camera->set_Rotation(cameraRotation);

        float aspectRatio = GetClientWidth() / static_cast<float>(GetClientHeight());
        m_Camera->set_Projection(m_Camera->get_FoV(), aspectRatio, SCREEN_NEAR, SCREEN_DEPTH);
    }

    {
        // Update the model matrix.
        float angle = static_cast<float>(e.TotalTime * 90.0);
        const XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);
        m_ModelMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));

        m_ViewMatrix = m_Camera->get_ViewMatrix();

        // Update the projection matrix.
        m_ProjectionMatrix = m_Camera->get_ProjectionMatrix();

        m_ViewProjectionMatrix = m_ViewMatrix * m_ProjectionMatrix;

        auto commandQueue = GetApp().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
        auto commandList = commandQueue->GetCommandList();
        m_skydoomRP->OnUpdate(commandList, e);
        m_perturbedCloudsRP.OnUpdate(commandList, e);
    }

    {
        m_Frustum.ConstructFrustum(SCREEN_DEPTH, m_Camera->get_ViewMatrix(), m_Camera->get_ProjectionMatrix());
    }
}

void ForwardPlusDemo::RescaleRenderTarget(float scale)
{
    uint32_t width = static_cast<uint32_t>(m_Width * scale);
    uint32_t height = static_cast<uint32_t>(m_Height * scale);

    width = std::clamp<uint32_t>(width, 1, D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);
    height = std::clamp<uint32_t>(height, 1, D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);

    m_RenderTarget.Resize(width, height);
}

void ForwardPlusDemo::OnRender(core::RenderEventArgs& e)
{
    super::OnRender(e);

    auto commandQueue = GetApp().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList = commandQueue->GetCommandList();

    // Clear the render targets.
    {
        FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

        commandList->ClearTexture(m_RenderTarget.GetTexture(core::AttachmentPoint::Color0), clearColor);
        commandList->ClearDepthStencilTexture(m_RenderTarget.GetTexture(core::AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH);
    }

    commandList->SetRenderTarget(m_RenderTarget);
    commandList->SetViewport(m_RenderTarget.GetViewport());
    commandList->SetScissorRect(m_ScissorRect);

    // Render the skybox.
    {
        m_skydoomRP->OnRender(commandList, e);
        m_perturbedCloudsRP.OnRender(commandList, e);
    }
 
    commandList->SetPipelineState(m_ScenePipelineState);
    commandList->SetGraphicsRootSignature(m_SceneRootSignature);
    //render scene
    {
        Mat matrices;
        auto model = XMMatrixScaling(1.f, 1.f, 1.f);
        ComputeMatrices(model, m_ViewMatrix, m_ProjectionMatrix, matrices);

        commandList->SetGraphicsDynamicConstantBuffer(static_cast<int>(SceneRootParameters::MatricesCB), matrices);
        commandList->SetGraphicsDynamicConstantBuffer(static_cast<int>(SceneRootParameters::DirLight), m_DirLight);
        commandList->SetShaderResourceView(static_cast<int>(SceneRootParameters::AmbientTex), 0, m_TerrainTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        
        m_Scene.Render(commandList, m_Frustum);
    }
    
    commandList->SetRenderTarget(m_pWindow->GetRenderTarget());
    commandList->SetViewport(m_pWindow->GetRenderTarget().GetViewport());
    commandList->SetPipelineState(m_QuadPipelineState);
    commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->SetGraphicsRootSignature(m_QuadRootSignature);
    commandList->SetShaderResourceView(0, 0, m_RenderTarget.GetTexture(core::Color0), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    commandList->Draw(3);

    commandQueue->ExecuteCommandList(commandList);

    OnGUI();

    m_pWindow->Present();
}

void ForwardPlusDemo::OnKeyPressed(core::KeyEventArgs& e)
{
    super::OnKeyPressed(e);
    if (!ImGui::GetIO().WantCaptureKeyboard)
        switch (e.Key)
        {
        case KeyCode::Escape:
            GetApp().Quit(0);
            break;
        case KeyCode::Enter:
            if (e.Alt)
            {
        case KeyCode::F11:
            m_pWindow->ToggleFullscreen();
            break;
            }
        case KeyCode::V:
            m_pWindow->ToggleVSync();
            break;
        case KeyCode::R:
            // Reset camera transform
            //m_Camera.set_Translation(m_pAlignedCameraData->m_InitialCamPos);
            //m_Camera.set_Rotation(m_pAlignedCameraData->m_InitialCamRot);
            //m_Camera.set_FoV(m_pAlignedCameraData->m_InitialFov);
            m_Pitch = 0.0f;
            m_Yaw = 0.0f;
            break;
        case KeyCode::Up:
        case KeyCode::W:
            m_Forward = m_CameraStep;
            break;
        case KeyCode::Left:
        case KeyCode::A:
            m_Left = m_CameraStep;
            break;
        case KeyCode::Down:
        case KeyCode::S:
            m_Backward = m_CameraStep;
            break;
        case KeyCode::Right:
        case KeyCode::D:
            m_Right = m_CameraStep;
            break;
        case KeyCode::Q:
            m_Down = m_CameraStep;
            break;
        case KeyCode::E:
            m_Up = m_CameraStep;
            break;
        case KeyCode::Space:
            m_AnimateLights = !m_AnimateLights;
            break;
        case KeyCode::ShiftKey:
            m_Shift = true;
            break;
        }

    //m_CameraEuler.KeyProcessing(e);
}

void ForwardPlusDemo::OnKeyReleased(core::KeyEventArgs& e)
{
    super::OnKeyReleased(e);

    if (!ImGui::GetIO().WantCaptureKeyboard)
        switch (e.Key)
        {
        case KeyCode::Up:
        case KeyCode::W:
            m_Forward = 0.0f;
            break;
        case KeyCode::Left:
        case KeyCode::A:
            m_Left = 0.0f;
            break;
        case KeyCode::Down:
        case KeyCode::S:
            m_Backward = 0.0f;
            break;
        case KeyCode::Right:
        case KeyCode::D:
            m_Right = 0.0f;
            break;
        case KeyCode::Q:
            m_Down = 0.0f;
            break;
        case KeyCode::E:
            m_Up = 0.0f;
            break;
        case KeyCode::ShiftKey:
            m_Shift = false;
            break;
        } 

    //m_CameraEuler.KeyProcessing(e);
}

void ForwardPlusDemo::OnMouseMoved(core::MouseMotionEventArgs& e)
{
    super::OnMouseMoved(e);

    const float mouseSpeed = 0.1f;

    if (!ImGui::GetIO().WantCaptureMouse)
        if (e.LeftButton)
        {
            m_Pitch -= e.RelY * mouseSpeed;

            m_Pitch = std::clamp(m_Pitch, -90.0f, 90.0f);

            m_Yaw -= e.RelX * mouseSpeed;
        }
    //m_CameraEuler.MouseProcessing(e);
}

void ForwardPlusDemo::OnMouseWheel(core::MouseWheelEventArgs& e)
{
    if (!ImGui::GetIO().WantCaptureMouse)
    {
        auto fov = m_Camera->get_FoV();

        fov -= e.WheelDelta;
        fov = std::clamp(fov, 12.0f, 90.0f);

        m_Camera->set_FoV(fov);

        //m_CameraEuler.ScrollProcessing(e);
        //auto fov = m_CameraEuler.get_FoV();

        char buffer[256];
        sprintf_s(buffer, "FoV: %f\n", fov);
        OutputDebugStringA(buffer);
    }
}

void ForwardPlusDemo::OnDPIScaleChanged(core::DPIScaleEventArgs& e)
{
    ImGui::GetIO().FontGlobalScale = e.DPIScale;
}

void ForwardPlusDemo::OnGUI()
{
    static bool showDemoWindow = false;

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit", "Esc"))
            {
                GetApp().Quit();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("ImGui Demo", nullptr, &showDemoWindow);

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Options"))
        {
            bool vSync = m_pWindow->IsVSync();
            if (ImGui::MenuItem("V-Sync", "V", &vSync))
            {
                m_pWindow->SetVSync(vSync);
            }

            bool fullscreen = m_pWindow->IsFullScreen();
            if (ImGui::MenuItem("Full screen", "Alt+Enter", &fullscreen))
            {
                m_pWindow->SetFullscreen(fullscreen);
            }

            ImGui::EndMenu();
        }

        char buffer[256];

        {
            sprintf_s(buffer, _countof(buffer), "FPS: %.2f (%.2f ms)  ", m_FPS, 1.0 / (m_FPS * 1000.0 + 0.001));
            auto fpsTextSize = ImGui::CalcTextSize(buffer);
            ImGui::SameLine(ImGui::GetWindowWidth() - fpsTextSize.x);
            ImGui::Text(buffer);
        }

        ImGui::EndMainMenuBar();
    }

    if (showDemoWindow)
    {
        ImGui::ShowDemoWindow(&showDemoWindow);
    }
}

