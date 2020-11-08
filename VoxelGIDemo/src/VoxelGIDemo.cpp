#include <VoxelGIDemo.h>

#include <Application.h>
#include <CommandQueue.h>
#include <DX12LibPCH.h>
#include <Helpers.h>
#include <Window.h>
#include <Material.h>
#include <StringConstant.h>
#include <VoxelGIDemoUtils.h>

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


VoxelGIDemo::VoxelGIDemo(const std::wstring& name, int width, int height, bool vSync)
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
    
    m_Config = std::make_unique<core::Config>(ConfigPathStr);

    XMVECTOR cameraPos = XMVectorSet(50, 2, -7, 1);
    XMVECTOR cameraTarget = XMVectorSet(0, 5, 0, 1);   
    XMVECTOR cameraUp = XMVectorSet(0, 1, 0, 0);

    m_Camera.set_LookAt(cameraPos, cameraTarget, cameraUp);
    m_Camera.set_Projection(45.0f, static_cast<float>(width) / height, SCREEN_NEAR, SCREEN_DEPTH);

    m_ViewMatrix = m_Camera.get_ViewMatrix();
    m_ProjectionMatrix = m_Camera.get_ProjectionMatrix();

    m_pAlignedCameraData = (CameraData*)_aligned_malloc(sizeof(CameraData), 16);

    m_pAlignedCameraData->m_InitialCamPos = m_Camera.get_Translation();
    m_pAlignedCameraData->m_InitialCamRot = m_Camera.get_Rotation();
    m_pAlignedCameraData->m_InitialFov = m_Camera.get_FoV(); 
}

VoxelGIDemo::~VoxelGIDemo()
{
    _aligned_free(m_pAlignedCameraData);
}

bool VoxelGIDemo::LoadContent()
{
    auto& app = GetApp();
    auto& device = app.GetDevice();
    auto commandQueue = app.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    auto commandList = commandQueue->GetCommandList();

    auto scenePath = m_Config->GetRoot().GetPath(SceneFileNameStr).GetValueText<std::wstring>();
    m_Sponza.LoadFromFile(commandList, scenePath, false);

    // Create an HDR intermediate render target.
    DXGI_FORMAT HDRFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

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
        m_MaterialDrawFun = [&](std::shared_ptr<core::CommandList>& commandList, std::shared_ptr<core::Material>& material)
        {
            if (!material->GetAmbientTex().IsEmpty())
                commandList->SetShaderResourceView(static_cast<int>(SceneRootParameters::AmbientTex), 0, material->GetAmbientTex(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        };

        m_DepthBufferDrawFun = [&](std::shared_ptr<core::CommandList>& commandList, std::shared_ptr<core::Material>& material)
        {
            
        };

        m_VoxelGridFillDrawFun = [&](std::shared_ptr<core::CommandList>& commandList, std::shared_ptr<core::Material>& material)
        {
            const auto& ambientTex = material->GetAmbientTex();
            if (!ambientTex.IsEmpty())
                m_VoxelGrid.AttachAmbientTex(commandList, ambientTex);
        };
    }

    float swidth = GetClientWidth();
    float sheight = GetClientHeight();


    {
        fpdu::CollectLightsFromConfig(*m_Config, m_Lights);
    }

    {
        m_VoxelGrid.InitVoxelGrid(commandList);
    }

    {
        core::VoxelGridDebugRenderPassInfo info;
        info.rootSignatureVersion = featureData.HighestVersion;
        info.rtvFormats = m_RenderTarget.GetRenderTargetFormats();
        m_VoxelGridDebugRP.LoadContent(&info);
    }

    {
        core::EnvironmentMapRenderPassInfo envInfo;
        envInfo.commandList = commandList;
        envInfo.camera = &m_Camera;
        envInfo.cubeMapSize = 1024;
        envInfo.cubeMapDepthOrArraySize = 6;
        envInfo.texturePath = L"Assets/Textures/grace-new.hdr";
        envInfo.textureName = L"Grace Cathedral Cubemap";
        envInfo.rootSignatureVersion = featureData.HighestVersion;
        envInfo.rtvFormats = m_RenderTarget.GetRenderTargetFormats();
        m_envRenderPass.LoadContent(&envInfo);
    }

    {
        core::DepthBufferRenderPassInfo info;
        info.bufferW = swidth;
        info.bufferH = sheight;
        info.rootSignatureVersion = featureData.HighestVersion;
        m_DepthBufferRenderPass.LoadContent(&info);
    }

    {
        core::DebugDepthBufferRenderPassInfo info;
        info.commandList = commandList;
        info.rtvFormats = m_RenderTarget.GetRenderTargetFormats();
        info.rootSignatureVersion = featureData.HighestVersion;
        m_DebugDepthBufferRenderPass.LoadContent(&info);
    }

    {
        core::QuadRenderPassInfo info;
        info.rootSignatureVersion = featureData.HighestVersion;
        info.rtvFormats = m_RenderTarget.GetRenderTargetFormats();
        m_QuadRenderPass.LoadContent(&info);
    }

    // Create a root signature for the forward shading (scene) pipeline.
    {
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

void VoxelGIDemo::UnloadContent()
{
    m_ContentLoaded = false;
}


void VoxelGIDemo::OnResize(core::ResizeEventArgs& e)
{
    super::OnResize(e);

    if (m_Width != e.Width || m_Height != e.Height)
    {
        m_Width = std::max(1, e.Width);
        m_Height = std::max(1, e.Height);

        float fov = m_Camera.get_FoV();
        float aspectRatio = m_Width / (float)m_Height;
        m_Camera.set_Projection(fov, aspectRatio, 0.1f, 100.0f);

        RescaleRenderTarget(m_RenderScale);
    }
}

void VoxelGIDemo::OnUpdate(core::UpdateEventArgs& e)
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
        // Update the camera.
        float speedMultipler = (m_Shift ? 16.0f : 4.0f);

        XMVECTOR cameraTranslate = XMVectorSet(m_Right - m_Left, 0.0f, m_Forward - m_Backward, 1.0f) * speedMultipler * static_cast<float>(e.ElapsedTime);
        XMVECTOR cameraPan = XMVectorSet(0.0f, m_Up - m_Down, 0.0f, 1.0f) * speedMultipler * static_cast<float>(e.ElapsedTime);
        m_Camera.Translate(cameraTranslate, core::ESpace::Local);
        m_Camera.Translate(cameraPan, core::ESpace::Local);

        XMVECTOR cameraRotation = XMQuaternionRotationRollPitchYaw(XMConvertToRadians(m_Pitch), XMConvertToRadians(m_Yaw), 0.0f);
        m_Camera.set_Rotation(cameraRotation);

        float aspectRatio = GetClientWidth() / static_cast<float>(GetClientHeight());
        m_Camera.set_Projection(m_Camera.get_FoV(), aspectRatio, SCREEN_NEAR, SCREEN_DEPTH);
    }
    {
        auto commandQueue = GetApp().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
        auto commandList = commandQueue->GetCommandList();
        m_envRenderPass.OnUpdate(commandList, e);
    }

    {
        // Update the model matrix.
        float angle = static_cast<float>(e.TotalTime * 90.0);
        const XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);
        m_ModelMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));

        m_ViewMatrix = m_Camera.get_ViewMatrix();

        // Update the projection matrix.
        m_ProjectionMatrix = m_Camera.get_ProjectionMatrix();

        m_ViewProjMatrix = m_ViewMatrix * m_ProjectionMatrix;
        m_InvViewProjMatrix = XMMatrixInverse(nullptr, m_ViewProjMatrix);
    }

    {
        m_Frustum.ConstructFrustum(SCREEN_DEPTH, m_Camera.get_ViewMatrix(), m_Camera.get_ProjectionMatrix());
    }
}

void VoxelGIDemo::RescaleRenderTarget(float scale)
{
    uint32_t width = static_cast<uint32_t>(m_Width * scale);
    uint32_t height = static_cast<uint32_t>(m_Height * scale);

    width = std::clamp<uint32_t>(width, 1, D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);
    height = std::clamp<uint32_t>(height, 1, D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);

    m_RenderTarget.Resize(width, height);
}

void VoxelGIDemo::OnRender(core::RenderEventArgs& e)
{
    super::OnRender(e);

    auto commandQueue = GetApp().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList = commandQueue->GetCommandList();

    // Clear the render targets.
    {
        FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

        commandList->ClearTexture(m_RenderTarget.GetTexture(core::AttachmentPoint::Color0), clearColor);
        commandList->ClearDepthStencilTexture(m_RenderTarget.GetTexture(core::AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL);
    }

    Mat matrices;
    //auto model = XMMatrixScaling(9.999999776e-003, 9.999999776e-003, 9.999999776e-003);
    auto model = XMMatrixScaling(1, 1, 1);
    ComputeMatrices(model, m_ViewMatrix, m_ProjectionMatrix, matrices);

    m_VoxelGrid.UpdateGrid(commandList, m_Camera);
    m_Sponza.Render(commandList, m_VoxelGridFillDrawFun);

    auto& device = GetApp().GetDevice();

    ComPtr<ID3D12Resource> mReadBackBuffer;
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
        D3D12_HEAP_FLAG_NONE,
        //&CD3DX12_RESOURCE_DESC::Buffer(m_NumLights.NUM_LIGHTS * sizeof(Light)),
        &CD3DX12_RESOURCE_DESC::Buffer(sizeof(core::VoxelOfVoxelGridClass) * 32 * 32 * 32),
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&mReadBackBuffer)));

    commandList->TransitionBarrier(m_VoxelGrid.GetVoxelGrid().GetD3D12Resource(), D3D12_RESOURCE_STATE_COPY_SOURCE);
    commandList->CopyResource(mReadBackBuffer, m_VoxelGrid.GetVoxelGrid().GetD3D12Resource());

    auto fenceValue = commandQueue->ExecuteCommandList(commandList);
    commandQueue->WaitForFenceValue(fenceValue);

    commandList = commandQueue->GetCommandList();

    core::VoxelOfVoxelGridClass* mappedData = nullptr;
    ThrowIfFailed(mReadBackBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedData)));
    std::vector<core::VoxelOfVoxelGridClass> sega;
    for (int i = 0; i < 32 * 32 * 32; ++i)
        sega.push_back(*(mappedData + i));

    mReadBackBuffer->Unmap(0, nullptr);
    //*/

    //commandQueue->ExecuteCommandList(commandList);
    //commandList = commandQueue->GetCommandList();

    {
        m_DepthBufferRenderPass.OnRender(commandList, e);
        commandList->SetGraphicsDynamicConstantBuffer(static_cast<int>(SceneRootParameters::MatricesCB), matrices);     
        m_Sponza.Render(commandList, m_Frustum, m_DepthBufferDrawFun);
    }


    if (m_Mode == EDemoMode::DepthBufferDebug)
    {
        commandList->SetRenderTarget(m_RenderTarget);
        commandList->SetViewport(m_RenderTarget.GetViewport());
        commandList->SetScissorRect(m_ScissorRect);
        m_DebugDepthBufferRenderPass.OnPreRender(commandList, e);
        commandList->TransitionBarrier(m_DepthBufferRenderPass.GetDepthBuffer(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->SetShaderResourceView(0, 0, m_DepthBufferRenderPass.GetDepthBuffer(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            0, 0, &m_DepthBufferRenderPass.GetSRV());
        m_DebugDepthBufferRenderPass.OnRender(commandList, e);
    }
    else if (m_Mode == EDemoMode::VoxelGridDebug)
    {
        commandList->SetRenderTarget(m_RenderTarget);
        commandList->SetViewport(m_RenderTarget.GetViewport());
        commandList->SetScissorRect(m_ScissorRect);
        m_VoxelGridDebugRP.OnPreRender(commandList, e);
        commandList->TransitionBarrier(m_DepthBufferRenderPass.GetDepthBuffer(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        commandList->SetShaderResourceView(2, 0, m_DepthBufferRenderPass.GetDepthBuffer(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            0, 0, &m_DepthBufferRenderPass.GetSRV());
        m_VoxelGridDebugRP.AttachVoxelGrid(commandList, m_VoxelGrid.GetVoxelGrid());
        m_VoxelGridDebugRP.AttachVoxelGridParams(commandList, m_VoxelGrid);
        m_VoxelGridDebugRP.AttachInvViewProjMatrix(commandList, m_InvViewProjMatrix);
        m_VoxelGridDebugRP.OnRender(commandList, e);
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

void VoxelGIDemo::OnKeyPressed(core::KeyEventArgs& e)
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
            m_Camera.set_Translation(m_pAlignedCameraData->m_InitialCamPos);
            m_Camera.set_Rotation(m_pAlignedCameraData->m_InitialCamRot);
            m_Camera.set_FoV(m_pAlignedCameraData->m_InitialFov);
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

}

void VoxelGIDemo::OnKeyReleased(core::KeyEventArgs& e)
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
}

void VoxelGIDemo::OnMouseMoved(core::MouseMotionEventArgs& e)
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
}

void VoxelGIDemo::OnMouseWheel(core::MouseWheelEventArgs& e)
{
    if (!ImGui::GetIO().WantCaptureMouse)
    {
        auto fov = m_Camera.get_FoV();

        fov -= e.WheelDelta;
        fov = std::clamp(fov, 12.0f, 90.0f);

        m_Camera.set_FoV(fov);

        char buffer[256];
        sprintf_s(buffer, "FoV: %f\n", fov);
        OutputDebugStringA(buffer);
    }
}

void VoxelGIDemo::OnDPIScaleChanged(core::DPIScaleEventArgs& e)
{
    ImGui::GetIO().FontGlobalScale = e.DPIScale;
}

void VoxelGIDemo::OnGUI()
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

            if (ImGui::MenuItem("VoxelGrid debug", "*", m_Mode == EDemoMode::VoxelGridDebug))
            {
                m_Mode = EDemoMode::VoxelGridDebug;
            }
            if (ImGui::MenuItem("Depth buffer debug", "*", m_Mode == EDemoMode::DepthBufferDebug))
            {
                m_Mode = EDemoMode::DepthBufferDebug;
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

