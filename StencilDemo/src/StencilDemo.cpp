#include <StencilDemo.h>

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

void XM_CALLCONV ComputeMatrices(const XMMATRIX& model, const CXMMATRIX& view, const CXMMATRIX& projection, stdu::Mat& mat)
{
    mat.ModelMatrix = model;
    mat.ModelViewMatrix = model * view;
    mat.InverseTransposeModelViewMatrix = DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(nullptr, mat.ModelViewMatrix));
    mat.ModelViewProjectionMatrix = mat.ModelViewMatrix * projection;
}

StencilDemo::StencilDemo(const std::wstring& name, int width, int height, bool vSync)
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
    XMVECTOR cameraPos = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
    XMVECTOR cameraTarget = XMVectorSet(0, 5, 0, 1);
    XMVECTOR cameraUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    m_Camera.set_LookAt(cameraPos, cameraTarget, cameraUp);
    m_Camera.set_Projection(45.0f, static_cast<float>(width) / height, SCREEN_NEAR, SCREEN_DEPTH);

    m_ViewMatrix = m_Camera.get_ViewMatrix();
    m_ProjectionMatrix = m_Camera.get_ProjectionMatrix();

    m_pAlignedCameraData = (CameraData*)_aligned_malloc(sizeof(CameraData), 16);

    m_pAlignedCameraData->m_InitialCamPos = m_Camera.get_Translation();
    m_pAlignedCameraData->m_InitialCamRot = m_Camera.get_Rotation();
    m_pAlignedCameraData->m_InitialFov = m_Camera.get_FoV();

    m_PassData.numDirLight = 3;
    m_PassData.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };

    m_DirLight.resize(m_PassData.numDirLight);

    m_DirLight[0].Strength = { 0.6f, 0.6f, 0.6f, 1.f };
    m_DirLight[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
    m_DirLight[1].Strength = { 0.3f, 0.3f, 0.3f, 1.f };
    m_DirLight[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
    m_DirLight[2].Strength = { 0.15f, 0.15f, 0.15f, 1.f };
    m_DirLight[2].Direction = { 0.0f, -0.707f, -0.707f };

    m_ReflectionDirLight = m_DirLight;

    for (int i = 0; i < m_PassData.numDirLight; ++i)
    {
        XMMATRIX R = DirectX::XMMatrixReflect(m_MirrorPlane);
        XMVECTOR lightDir = XMLoadFloat3(&m_DirLight[i].Direction);
        XMVECTOR reflectedLightDir = XMVector3TransformNormal(lightDir, R);
        XMStoreFloat3(&m_ReflectionDirLight[i].Direction, reflectedLightDir);
    }
}

StencilDemo::~StencilDemo()
{
    _aligned_free(m_pAlignedCameraData);
}

bool StencilDemo::LoadContent()
{
    auto& app = GetApp();
    auto& device = app.GetDevice();
    auto commandQueue = app.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    auto commandList = commandQueue->GetCommandList();

    // Create an HDR intermediate render target.
    DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    // Create an off-screen render target with a single color buffer and a depth buffer.
    auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(BackBufferFormat, m_Width, m_Height);
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
        m_Textures[ETexture::bricksTex] = core::Texture();
        commandList->LoadTextureFromFile(m_Textures[ETexture::bricksTex], L"Assets/Textures/bricks3.dds");

        m_Textures[ETexture::checkboardTex] = core::Texture();
        commandList->LoadTextureFromFile(m_Textures[ETexture::checkboardTex], L"Assets/Textures/checkboard.dds");

        m_Textures[ETexture::iceTex] = core::Texture();
        commandList->LoadTextureFromFile(m_Textures[ETexture::iceTex], L"Assets/Textures/ice.dds");

        m_Textures[ETexture::white1x1Tex] = core::Texture();
        commandList->LoadTextureFromFile(m_Textures[ETexture::white1x1Tex], L"Assets/Textures/white1x1.dds");
    }

    {
        m_Materials[EMaterial::bricks] = stdu::MaterialConstant();
        auto& briks = m_Materials[EMaterial::bricks];
        briks.diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        briks.freshnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
        briks.Roughness = 0.25f;

        m_Materials[EMaterial::checkertile] = stdu::MaterialConstant();
        auto& checkertile = m_Materials[EMaterial::checkertile];
        checkertile.diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        checkertile.freshnelR0 = XMFLOAT3(0.07f, 0.07f, 0.07f);
        checkertile.Roughness = 0.3f;

        m_Materials[EMaterial::icemirror] = stdu::MaterialConstant();
        auto& icemirror = m_Materials[EMaterial::icemirror];
        icemirror.diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.3f);
        icemirror.freshnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
        icemirror.Roughness = 0.5f;

        m_Materials[EMaterial::skullMat] = stdu::MaterialConstant();
        auto& skullMat = m_Materials[EMaterial::skullMat];
        skullMat.diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        skullMat.freshnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
        skullMat.Roughness = 0.3f;

        m_Materials[EMaterial::shadowMat] = stdu::MaterialConstant();
        auto& shadowMat = m_Materials[EMaterial::shadowMat];
        shadowMat.diffuseAlbedo = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f);
        shadowMat.freshnelR0 = XMFLOAT3(0.001f, 0.001f, 0.001f);
        shadowMat.Roughness = 0.0f;
    }

    {
        m_Meshes[EMeshes::rooms] = stdu::BuildRoomMesh(*commandList);
        m_Meshes[EMeshes::skull] = stdu::LoadMeshFromFile(*commandList, "Assets/models/skull_frank_luna/skull.txt");
    }

    {
        m_RenderItems[ERenderLayer::Opaque] = {};
        m_RenderItems[ERenderLayer::Mirrors] = {};
        m_RenderItems[ERenderLayer::Reflected] = {};
        m_RenderItems[ERenderLayer::Transparent] = {};
        m_RenderItems[ERenderLayer::Shadow] = {};
    }

    {
        stdu::RenderItem floorRitem;
        floorRitem.mat_index = static_cast<int16_t>(EMaterial::checkertile);
        floorRitem.tex_index = static_cast<int16_t>(ETexture::checkboardTex);
        floorRitem.mesh_index = static_cast<int16_t>(EMeshes::rooms);
        floorRitem.bUseSubmesh = true;
        floorRitem.submesh_index = static_cast<int16_t>(stdu::ERoomPart::floor);
        m_RenderItems[ERenderLayer::Opaque].push_back(floorRitem);

        stdu::RenderItem wallRitem;
        wallRitem.mat_index = static_cast<int16_t>(EMaterial::bricks);
        wallRitem.tex_index = static_cast<int16_t>(ETexture::bricksTex);
        wallRitem.mesh_index = static_cast<int16_t>(EMeshes::rooms);
        wallRitem.bUseSubmesh = true;
        wallRitem.submesh_index = static_cast<int16_t>(stdu::ERoomPart::wall);
        m_RenderItems[ERenderLayer::Opaque].push_back(wallRitem);

        stdu::RenderItem skullRitem;
        skullRitem.mat_index = static_cast<int16_t>(EMaterial::skullMat);
        skullRitem.tex_index = static_cast<int16_t>(ETexture::white1x1Tex);
        skullRitem.mesh_index = static_cast<int16_t>(EMeshes::skull);
        skullRitem.bUseSubmesh = false;
        skullRitem.bUseCustomWoldMatrix = true;
        XMMATRIX skullRotate = DirectX::XMMatrixRotationY(0.5f * M_PI);
        XMFLOAT3 skullTranslation = { 0.0f, 1.0f, -5.0f };
        XMMATRIX skullScale = DirectX::XMMatrixScaling(0.45f, 0.45f, 0.45f);
        XMMATRIX skullOffset = DirectX::XMMatrixTranslation(skullTranslation.x, skullTranslation.y, skullTranslation.z);
        skullRitem.worldMatrix = skullRotate * skullScale * skullOffset;
        m_RenderItems[ERenderLayer::Opaque].push_back(skullRitem);

        stdu::RenderItem reflectedSkullRitem = skullRitem;
        XMMATRIX reflectionMatrix = DirectX::XMMatrixReflect(m_MirrorPlane);
        reflectedSkullRitem.worldMatrix = DirectX::XMMatrixMultiply(skullRitem.worldMatrix, reflectionMatrix);
        m_RenderItems[ERenderLayer::Reflected].push_back(reflectedSkullRitem);

        stdu::RenderItem shadowedSkullRitem = skullRitem;
        shadowedSkullRitem.mat_index = static_cast<int16_t>(EMaterial::shadowMat);
        XMVECTOR toMainLight = -XMLoadFloat3(&m_DirLight[0].Direction);
        XMMATRIX S = DirectX::XMMatrixShadow(m_ShadowPlane, toMainLight);
        XMMATRIX shadowOffsetY = DirectX::XMMatrixTranslation(0.0f, 0.001f, 0.0f);
        shadowedSkullRitem.worldMatrix =  skullRitem.worldMatrix * S * shadowOffsetY;
        m_RenderItems[ERenderLayer::Shadow].push_back(shadowedSkullRitem);

        stdu::RenderItem mirrorRitem;
        mirrorRitem.mat_index = static_cast<int16_t>(EMaterial::icemirror);
        mirrorRitem.tex_index = static_cast<int16_t>(ETexture::iceTex);
        mirrorRitem.mesh_index = static_cast<int16_t>(EMeshes::rooms);
        mirrorRitem.bUseSubmesh = true;
        mirrorRitem.submesh_index = static_cast<int16_t>(stdu::ERoomPart::mirror);
        m_RenderItems[ERenderLayer::Mirrors].push_back(mirrorRitem);
        m_RenderItems[ERenderLayer::Transparent].push_back(mirrorRitem);
    }

    {
        //root signature desc
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

        CD3DX12_ROOT_PARAMETER1 rootParameters[static_cast<size_t>(stdu::SceneRootParameters::NumRootParameters)];
        rootParameters[static_cast<int>(stdu::SceneRootParameters::MatricesCB)].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
        rootParameters[static_cast<int>(stdu::SceneRootParameters::DirLight)].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[static_cast<int>(stdu::SceneRootParameters::Materials)].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[static_cast<int>(stdu::SceneRootParameters::RenderPassData)].InitAsConstantBufferView(2, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
        CD3DX12_DESCRIPTOR_RANGE1 ambientTexDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
        rootParameters[static_cast<int>(stdu::SceneRootParameters::AmbientTex)].InitAsDescriptorTable(1, &ambientTexDescrRange, D3D12_SHADER_VISIBILITY_PIXEL);

        CD3DX12_STATIC_SAMPLER_DESC anisotropicWrapSampler(
            0, // shaderRegister
            D3D12_FILTER_ANISOTROPIC, // filter
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
            0.0f,                             // mipLODBias
            8);                               // maxAnisotropy
        anisotropicWrapSampler.MaxLOD = 0;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
        rootSignatureDescription.Init_1_1(static_cast<size_t>(stdu::SceneRootParameters::NumRootParameters), rootParameters, 1, &anisotropicWrapSampler, rootSignatureFlags);

        m_SceneRootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

        // for opaque pso
        D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoStream;
        ZeroMemory(&opaquePsoStream, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
        opaquePsoStream.pRootSignature = m_SceneRootSignature.GetRootSignature().Get();
        opaquePsoStream.InputLayout = { core::PosNormTexVertex::InputElements, core::PosNormTexVertex::InputElementCount };
        opaquePsoStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        opaquePsoStream.VS = CD3DX12_SHADER_BYTECODE(vs.Get());
        opaquePsoStream.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
        opaquePsoStream.SampleMask = UINT_MAX;
        opaquePsoStream.NumRenderTargets = 1;
        opaquePsoStream.SampleDesc.Count = 1;
        opaquePsoStream.SampleDesc.Quality = 0;
        opaquePsoStream.DSVFormat = m_RenderTarget.GetDepthStencilFormat();
        opaquePsoStream.RTVFormats[0] = m_RenderTarget.GetRenderTargetFormats().RTFormats[0];
        opaquePsoStream.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        opaquePsoStream.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        opaquePsoStream.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

        ThrowIfFailed(device->CreateGraphicsPipelineState(&opaquePsoStream, IID_PPV_ARGS(&m_OpaqueScenePipelineState)));

        // for transparent pso
        D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoStream = opaquePsoStream;

        D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
        transparencyBlendDesc.BlendEnable = true;
        transparencyBlendDesc.LogicOpEnable = false;
        transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
        transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
        transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
        transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
        transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        transparentPsoStream.BlendState.RenderTarget[0] = transparencyBlendDesc;

        ThrowIfFailed(device->CreateGraphicsPipelineState(&transparentPsoStream, IID_PPV_ARGS(&m_TransparentScenePipelineState)));

        //PSO for marking stencil mirrors.
        CD3DX12_BLEND_DESC mirrorBlendState(D3D12_DEFAULT);
        mirrorBlendState.RenderTarget[0].RenderTargetWriteMask = FALSE;

        D3D12_DEPTH_STENCIL_DESC mirrorDSS;
        mirrorDSS.DepthEnable = true;
        mirrorDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        mirrorDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        mirrorDSS.StencilEnable = true;
        mirrorDSS.StencilReadMask = 0xff; // 255
        mirrorDSS.StencilWriteMask = 0xff;

        mirrorDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        mirrorDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        mirrorDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
        mirrorDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

        // We are not rendering backfacing polygons, so these settings do not matter.
        mirrorDSS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        mirrorDSS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        mirrorDSS.BackFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
        mirrorDSS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC markMirrorsPsoDesc = opaquePsoStream;
        markMirrorsPsoDesc.BlendState = mirrorBlendState;
        markMirrorsPsoDesc.DepthStencilState = mirrorDSS;

        ThrowIfFailed(device->CreateGraphicsPipelineState(&markMirrorsPsoDesc, IID_PPV_ARGS(&m_MarkStencilMirrorsScenePipelineState)));

        //PSO for stencil reflections.
        D3D12_DEPTH_STENCIL_DESC reflectionsDSS;
        reflectionsDSS.DepthEnable = true;
        reflectionsDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        reflectionsDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        reflectionsDSS.StencilEnable = true;
        reflectionsDSS.StencilReadMask = 0xff;
        reflectionsDSS.StencilWriteMask = 0xff;

        reflectionsDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        reflectionsDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        reflectionsDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        reflectionsDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

        // We are not rendering backfacing polygons, so these settings do not matter.
        reflectionsDSS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        reflectionsDSS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        reflectionsDSS.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        reflectionsDSS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC drawReflectionsPsoDesc = opaquePsoStream;
        drawReflectionsPsoDesc.DepthStencilState = reflectionsDSS;
        drawReflectionsPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        drawReflectionsPsoDesc.RasterizerState.FrontCounterClockwise = true;

        ThrowIfFailed(device->CreateGraphicsPipelineState(&drawReflectionsPsoDesc, IID_PPV_ARGS(&m_DrawStencilReflectionsScenePipelineState)));

        //PSO for shadow objects
        D3D12_DEPTH_STENCIL_DESC shadowDSS;
        shadowDSS.DepthEnable = true;
        shadowDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        shadowDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        shadowDSS.StencilEnable = true;
        shadowDSS.StencilReadMask = 0xff;
        shadowDSS.StencilWriteMask = 0xff;

        shadowDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        shadowDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        shadowDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
        shadowDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

        // We are not rendering backfacing polygons, so these settings do not matter.
        shadowDSS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        shadowDSS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        shadowDSS.BackFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
        shadowDSS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowPsoDesc = transparentPsoStream;
        shadowPsoDesc.DepthStencilState = shadowDSS;

        ThrowIfFailed(device->CreateGraphicsPipelineState(&shadowPsoDesc, IID_PPV_ARGS(&m_ShadowScenePipelineState)));
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

void StencilDemo::UnloadContent()
{
    m_ContentLoaded = false;
}


void StencilDemo::OnResize(core::ResizeEventArgs& e)
{
    super::OnResize(e);

    if (m_Width != e.Width || m_Height != e.Height)
    {
        m_Width = std::max(1, e.Width);
        m_Height = std::max(1, e.Height);

        float fov = m_Camera.get_FoV();
        float aspectRatio = m_Width / (float)m_Height;
        m_Camera.set_Projection(fov, aspectRatio, 0.1f, 100.0f);
        //m_CameraEuler.set_Projection(m_CameraEuler.get_FoV(), aspectRatio, SCREEN_NEAR, SCREEN_DEPTH);

        RescaleRenderTarget(m_RenderScale);
    }
}

void StencilDemo::OnUpdate(core::UpdateEventArgs& e)
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
        auto& cameraPos = m_Camera.get_Translation();
        XMStoreFloat3(&m_PassData.ViewPos, cameraPos);
    }

    {
        // Update the model matrix.
        float angle = static_cast<float>(e.TotalTime * 90.0);
        const XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);
        m_ModelMatrix = DirectX::XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));

        m_ViewMatrix = m_Camera.get_ViewMatrix();

        // Update the projection matrix.
        m_ProjectionMatrix = m_Camera.get_ProjectionMatrix();
    }
}

void StencilDemo::RescaleRenderTarget(float scale)
{
    uint32_t width = static_cast<uint32_t>(m_Width * scale);
    uint32_t height = static_cast<uint32_t>(m_Height * scale);

    width = std::clamp<uint32_t>(width, 1, D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);
    height = std::clamp<uint32_t>(height, 1, D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);

    m_RenderTarget.Resize(width, height);
}

void StencilDemo::OnRender(core::RenderEventArgs& e)
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

    commandList->SetRenderTarget(m_RenderTarget);
    commandList->SetViewport(m_RenderTarget.GetViewport());
    commandList->SetScissorRect(m_ScissorRect);

    commandList->SetGraphicsRootSignature(m_SceneRootSignature);
    //render scene
    {
        commandList->SetPipelineState(m_OpaqueScenePipelineState);
        stdu::Mat matrices;
        auto model = DirectX::XMMatrixScaling(1.f, 1.f, 1.f);
        ComputeMatrices(model, m_ViewMatrix, m_ProjectionMatrix, matrices);
        commandList->SetGraphicsDynamicStructuredBuffer(static_cast<int>(stdu::SceneRootParameters::DirLight), m_DirLight);
        commandList->SetGraphicsDynamicConstantBuffer(static_cast<int>(stdu::SceneRootParameters::RenderPassData), m_PassData);
        DrawRenderItem(commandList, m_RenderItems[ERenderLayer::Opaque], matrices);

        // Mark the visible mirror pixels in the stencil buffer with the value 1
        commandList->SetStencilRef(1);
        commandList->SetPipelineState(m_MarkStencilMirrorsScenePipelineState);
        DrawRenderItem(commandList, m_RenderItems[ERenderLayer::Mirrors], matrices);

        commandList->SetPipelineState(m_DrawStencilReflectionsScenePipelineState);
        commandList->SetGraphicsDynamicStructuredBuffer(static_cast<int>(stdu::SceneRootParameters::DirLight), m_ReflectionDirLight);
        DrawRenderItem(commandList, m_RenderItems[ERenderLayer::Reflected], matrices);

        commandList->SetGraphicsDynamicStructuredBuffer(static_cast<int>(stdu::SceneRootParameters::DirLight), m_DirLight);
        commandList->SetStencilRef(0);

        commandList->SetPipelineState(m_TransparentScenePipelineState);
        DrawRenderItem(commandList, m_RenderItems[ERenderLayer::Transparent], matrices);

        commandList->SetPipelineState(m_ShadowScenePipelineState);
        DrawRenderItem(commandList, m_RenderItems[ERenderLayer::Shadow], matrices);
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

void StencilDemo::DrawRenderItem(std::shared_ptr<core::CommandList>& commandList,
    const std::vector<stdu::RenderItem>& items, const stdu::Mat& matricesWithWorldIndentity)
{
    for (const auto& item : items)
    {
        if (item.bUseCustomWoldMatrix)
        {
            stdu::Mat matrices;
            ComputeMatrices(item.worldMatrix, m_ViewMatrix, m_ProjectionMatrix, matrices);
            commandList->SetGraphicsDynamicConstantBuffer(static_cast<int>(stdu::SceneRootParameters::MatricesCB), matrices);
        }
        else
        {
            commandList->SetGraphicsDynamicConstantBuffer(static_cast<int>(stdu::SceneRootParameters::MatricesCB), matricesWithWorldIndentity);
        }

        auto& ambientTex = m_Textures[static_cast<ETexture>(item.tex_index)];
        commandList->SetShaderResourceView(static_cast<int>(stdu::SceneRootParameters::AmbientTex), 0, ambientTex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        auto& materials = m_Materials[static_cast<EMaterial>(item.mat_index)];
        commandList->SetGraphicsDynamicConstantBuffer(static_cast<int>(stdu::SceneRootParameters::Materials), materials);

        auto& mesh = m_Meshes[static_cast<EMeshes>(item.mesh_index)];
        if (item.bUseSubmesh)
        {
            mesh->RenderSubMesh(commandList, item.submesh_index);
        }
        else
        {
            mesh->Render(commandList);
        }
    }
}

void StencilDemo::OnKeyPressed(core::KeyEventArgs& e)
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

void StencilDemo::OnKeyReleased(core::KeyEventArgs& e)
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

void StencilDemo::OnMouseMoved(core::MouseMotionEventArgs& e)
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

void StencilDemo::OnMouseWheel(core::MouseWheelEventArgs& e)
{
    if (!ImGui::GetIO().WantCaptureMouse)
    {
        auto fov = m_Camera.get_FoV();

        fov -= e.WheelDelta;
        fov = std::clamp(fov, 12.0f, 90.0f);

        m_Camera.set_FoV(fov);

        //m_CameraEuler.ScrollProcessing(e);
        //auto fov = m_CameraEuler.get_FoV();

        char buffer[256];
        sprintf_s(buffer, "FoV: %f\n", fov);
        OutputDebugStringA(buffer);
    }
}

void StencilDemo::OnDPIScaleChanged(core::DPIScaleEventArgs& e)
{
    ImGui::GetIO().FontGlobalScale = e.DPIScale;
}

void StencilDemo::OnGUI()
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

