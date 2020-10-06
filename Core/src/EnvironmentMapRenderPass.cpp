#include <EnvironmentMapRenderPass.h>

#include <DX12LibPCH.h>

#include <Skybox_PS.h>
#include <Skybox_VS.h>

#include <Application.h>

using namespace dx12demo::core;

EnvironmentMapRenderPass::EnvironmentMapRenderPass()
{

}

EnvironmentMapRenderPass::~EnvironmentMapRenderPass()
{

}

void EnvironmentMapRenderPass::LoadContent(RenderPassBaseInfo* info)
{
	EnvironmentMapRenderPassInfo* envInfo = dynamic_cast<EnvironmentMapRenderPassInfo*>(info);

    m_Camera = &*envInfo->camera;
	// Create an inverted (reverse winding order) cube so the insides are not clipped.
	m_SkyboxMesh = core::Mesh::CreateCube(*envInfo->commandList, 1.0f, true);

	envInfo->commandList->LoadTextureFromFile(m_SrcTexture, envInfo->texturePath);

    auto cubemapDesc = m_SrcTexture.GetD3D12ResourceDesc();
    cubemapDesc.Width = cubemapDesc.Height = envInfo->cubeMapSize;
    cubemapDesc.DepthOrArraySize = envInfo->cubeMapDepthOrArraySize;
    cubemapDesc.MipLevels = envInfo->cubeMapMipLevels;

    m_CubemapTexture = core::Texture(cubemapDesc, nullptr, TextureUsage::Albedo, envInfo->textureName);
    // Convert the 2D panorama to a 3D cubemap.
    envInfo->commandList->PanoToCubemap(m_CubemapTexture, m_SrcTexture);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = m_CubemapTexture.GetD3D12ResourceDesc().Format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.TextureCube.MipLevels = (UINT)-1; // Use all mips.
    m_SrvDesc = srvDesc;

    // Create a root signature and PSO for the skybox shaders.
    {
        D3D12_INPUT_ELEMENT_DESC inputLayout[1] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        // Allow input layout and deny unnecessary access to certain pipeline stages.
        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
        CD3DX12_ROOT_PARAMETER1 rootParameters[2];
        rootParameters[0].InitAsConstants(sizeof(DirectX::XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        rootParameters[1].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

        CD3DX12_STATIC_SAMPLER_DESC linearClampSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
        rootSignatureDescription.Init_1_1(2, rootParameters, 1, &linearClampSampler, rootSignatureFlags);

        m_SkyboxSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, envInfo->rootSignatureVersion);

        // Setup the Skybox pipeline state.
        struct SkyboxPipelineState
        {
            CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
            CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
            CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
            CD3DX12_PIPELINE_STATE_STREAM_VS VS;
            CD3DX12_PIPELINE_STATE_STREAM_PS PS;
            CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
        } skyboxPipelineStateStream;

        skyboxPipelineStateStream.pRootSignature = m_SkyboxSignature.GetRootSignature().Get();
        skyboxPipelineStateStream.InputLayout = { inputLayout, 1 };
        skyboxPipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        skyboxPipelineStateStream.VS = { g_Skybox_VS, sizeof(g_Skybox_VS) };
        skyboxPipelineStateStream.PS = { g_Skybox_PS, sizeof(g_Skybox_PS) };
        skyboxPipelineStateStream.RTVFormats = envInfo->rtvFormats;

        D3D12_PIPELINE_STATE_STREAM_DESC skyboxPipelineStateStreamDesc = {
            sizeof(SkyboxPipelineState), &skyboxPipelineStateStream
        };

        auto& device = GetApp().GetDevice();
        ThrowIfFailed(device->CreatePipelineState(&skyboxPipelineStateStreamDesc, IID_PPV_ARGS(&m_SkyboxPipelineState)));
    }
}

void EnvironmentMapRenderPass::OnUpdate(CommandList& commandList, UpdateEventArgs& e)
{
    auto viewMatrix = XMMatrixTranspose(XMMatrixRotationQuaternion(m_Camera->get_Rotation()));
    auto& projMatrix = m_Camera->get_ProjectionMatrix();
    m_ViewProjMatrix = viewMatrix * projMatrix;
}

void EnvironmentMapRenderPass::OnRender(CommandList& commandList, RenderEventArgs& e)
{
    commandList.SetPipelineState(m_SkyboxPipelineState);
    commandList.SetGraphicsRootSignature(m_SkyboxSignature);

    commandList.SetGraphics32BitConstants(0, m_ViewProjMatrix);

    commandList.SetShaderResourceView(1, 0, m_CubemapTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, &m_SrvDesc);

    m_SkyboxMesh->Render(commandList);
}