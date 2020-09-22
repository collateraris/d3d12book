#include <Lesson1_cube3d.h>

#include <Application.h>
#include <CommandQueue.h>
#include <DX12LibPCH.h>
#include <Helpers.h>
#include <Window.h>

using namespace dx12demo;
using namespace DirectX;

// An enum for root signature parameters.
// I'm not using scoped enums to avoid the explicit cast that would be required
// to use these as root indices in the root signature.
enum RootParameters
{
    MatricesCB,         // ConstantBuffer<Mat> MatCB : register(b0);
    Textures,           // Texture2D DiffuseTexture : register( t2 );
    NumRootParameters
};

struct ModelViewProjection
{
    XMMATRIX MVP;
};

Lesson1_cube3d::Lesson1_cube3d(const std::wstring& name, int width, int height, bool vSync)
    : super(name, width, height, vSync)
    , m_ScissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX))
    , m_FoV(45.0)
    , m_ContentLoaded(false)
    , m_Width(0)
    , m_Height(0)
{
}

bool Lesson1_cube3d::LoadContent()
{
    auto& app = GetApp();
    auto& device = app.GetDevice();
    auto commandQueue = app.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    auto commandList = commandQueue->GetCommandList();

    m_SphereMesh = core::Mesh::CreateSphere(*commandList);
    commandList->LoadTextureFromFile(m_EarthTexture, L"Assets/Textures/earth.dds");

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

    // Create a root signature for the common pipeline.

    {
        // Load the  shaders.
        ComPtr<ID3DBlob> vs;
        ComPtr<ID3DBlob> ps;
        ThrowIfFailed(D3DReadFileToBlob(L"VertexShader.cso", &vs));
        ThrowIfFailed(D3DReadFileToBlob(L"PixelShader.cso", &ps));

        // Allow input layout and deny unnecessary access to certain pipeline stages.
        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);

        CD3DX12_ROOT_PARAMETER1 rootParameters[RootParameters::NumRootParameters];
        rootParameters[RootParameters::MatricesCB].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
        rootParameters[RootParameters::Textures].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

        CD3DX12_STATIC_SAMPLER_DESC linearRepeatSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
        rootSignatureDescription.Init_1_1(RootParameters::NumRootParameters, rootParameters, 1, &linearRepeatSampler, rootSignatureFlags);

        m_RootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

        struct PipelineStateStream
        {
            CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
            CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
            CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
            CD3DX12_PIPELINE_STATE_STREAM_VS VS;
            CD3DX12_PIPELINE_STATE_STREAM_PS PS;
            CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
            CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
        } pipelineStateStream;

        pipelineStateStream.pRootSignature = m_RootSignature.GetRootSignature().Get();
        pipelineStateStream.InputLayout = { core::VertexPositionNormalTexture::InputElements, core::VertexPositionNormalTexture::InputElementCount };
        pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vs.Get());
        pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
        pipelineStateStream.DSVFormat = m_RenderTarget.GetDepthStencilFormat();
        pipelineStateStream.RTVFormats = m_RenderTarget.GetRenderTargetFormats();

        D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
            sizeof(PipelineStateStream), &pipelineStateStream
        };
        ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState)));
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

void Lesson1_cube3d::UnloadContent()
{
    m_ContentLoaded = false;
}


void Lesson1_cube3d::OnResize(core::ResizeEventArgs& e)
{
    super::OnResize(e);

    if (m_Width != e.Width || m_Height != e.Height)
    {
        m_Width = std::max(1, e.Width);
        m_Height = std::max(1, e.Height);
    }
}

void Lesson1_cube3d::OnUpdate(core::UpdateEventArgs& e)
{
    static uint64_t frameCount = 0;
    static double totalTime = 0.0;

    super::OnUpdate(e);

    totalTime += e.ElapsedTime;
    frameCount++;

    if (totalTime > 1.0)
    {
        double fps = frameCount / totalTime;

        char buffer[512];
        sprintf_s(buffer, "FPS: %f\n", fps);
        OutputDebugStringA(buffer);

        frameCount = 0;
        totalTime = 0.0;
    }

    // Update the model matrix.
    float angle = static_cast<float>(e.TotalTime * 90.0);
    const XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);
    m_ModelMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));

    // Update the view matrix.
    const XMVECTOR eyePosition = XMVectorSet(0, 0, -10, 1);
    const XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
    const XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
    m_ViewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

    // Update the projection matrix.
    float aspectRatio = GetClientWidth() / static_cast<float>(GetClientHeight());
    m_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_FoV), aspectRatio, 0.1f, 100.0f);
}

void Lesson1_cube3d::OnRender(core::RenderEventArgs& e)
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

    commandList->SetPipelineState(m_PipelineState);
    commandList->SetGraphicsRootSignature(m_RootSignature);

    XMMATRIX mvpMatrix = XMMatrixMultiply(m_ModelMatrix, m_ViewMatrix);
    mvpMatrix = XMMatrixMultiply(mvpMatrix, m_ProjectionMatrix);
    ModelViewProjection MVP = {mvpMatrix};
    commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MatricesCB, MVP);
    commandList->SetShaderResourceView(RootParameters::Textures, 0, m_EarthTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    m_SphereMesh->Render(*commandList);

    commandList->SetRenderTarget(m_pWindow->GetRenderTarget());
    commandList->SetViewport(m_pWindow->GetRenderTarget().GetViewport());
    commandList->SetPipelineState(m_QuadPipelineState);
    commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->SetGraphicsRootSignature(m_QuadRootSignature);
    commandList->SetShaderResourceView(0, 0, m_RenderTarget.GetTexture(core::Color0), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    commandList->Draw(3);

    commandQueue->ExecuteCommandList(commandList);

    m_pWindow->Present();
}

void Lesson1_cube3d::OnKeyPressed(core::KeyEventArgs& e)
{

    super::OnKeyPressed(e);

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
    }
}

void Lesson1_cube3d::OnMouseWheel(core::MouseWheelEventArgs& e)
{
    m_FoV -= e.WheelDelta;
    m_FoV = std::clamp(m_FoV, 12.0f, 90.0f);

    char buffer[256];
    sprintf_s(buffer, "FoV: %f\n", m_FoV);
    OutputDebugStringA(buffer);
}

void Lesson1_cube3d::OnDPIScaleChanged(core::DPIScaleEventArgs& e)
{

}

