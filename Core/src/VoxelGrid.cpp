#include <VoxelGrid.h>

#include <DX12LibPCH.h>
#include <Application.h>
#include <CommandQueue.h>
#include <Mesh.h>

#include <VoxelGridFill_VS.h>
#include <VoxelGridFill_GS.h>
#include <VoxelGridFill_PS.h>


using namespace dx12demo::core;

namespace UpdateParams
{
    enum
    {
        b0MatCB,
        b1GridParamsCB,
        t0AmbientTexture,
        u0VoxelsGridSB,
        NumRootParameters
    };
}

VoxelGrid::VoxelGrid(int gridSize, float gridHalfExtent/* = 1000.0f*/, int renderTargetSize/* = 64*/)
    : m_GridNum{gridSize},
    m_GridHalfExtent{ gridHalfExtent },
    m_RenderTargetSize{ renderTargetSize }
{
    Init();
}

VoxelGrid::VoxelGrid()
{
    Init();
}

VoxelGrid::~VoxelGrid()
{
    
}

void VoxelGrid::Init()
{
    auto device = GetApp().GetDevice();

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    m_ScissorRect = { 0, 0, m_RenderTargetSize, m_RenderTargetSize };
    // Create an HDR intermediate render target.
    DXGI_FORMAT RTFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

    // Create an off-screen render target with a single color buffer and a depth buffer.
    auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(RTFormat, m_RenderTargetSize, m_RenderTargetSize);
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

    m_RenderTarget.AttachTexture(AttachmentPoint::Color0, ScreenTexture);

    //update grid 
    {
        CD3DX12_ROOT_PARAMETER1 rootParameters[UpdateParams::NumRootParameters];
        rootParameters[UpdateParams::b0MatCB].InitAsConstantBufferView(0, 0);
        rootParameters[UpdateParams::b1GridParamsCB].InitAsConstantBufferView(1, 0);
        CD3DX12_DESCRIPTOR_RANGE1 ambientTexDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
        rootParameters[UpdateParams::t0AmbientTexture].InitAsDescriptorTable(1, &ambientTexDescrRange);
        CD3DX12_DESCRIPTOR_RANGE1 voxelsGridDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
        rootParameters[UpdateParams::u0VoxelsGridSB].InitAsDescriptorTable(1, &voxelsGridDescrRange);

        // Allow input layout and deny unnecessary access to certain pipeline stages.
        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

        const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
            0, // shaderRegister
            D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
        rootSignatureDescription.Init_1_1(UpdateParams::NumRootParameters, rootParameters, 1, &linearClamp, rootSignatureFlags);

        m_RootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

        // disable depth-write and depth-test, in order to fully "voxelize" scene geometry 
        CD3DX12_DEPTH_STENCIL_DESC depthDesc(D3D12_DEFAULT);
        depthDesc.DepthEnable = false;
        depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

        D3D12_RENDER_TARGET_BLEND_DESC blendDesc;
        blendDesc.BlendEnable = true;
        blendDesc.LogicOpEnable = false;
        blendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        blendDesc.BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
        blendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
        blendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
        blendDesc.RenderTargetWriteMask = 0;

        // disable color-write since instead of outputting the rasterized voxel information into the bound render-target,
        // it will be written into a 3D structured buffer/ texture 
        CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
        //rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateStream;
        ZeroMemory(&pipelineStateStream, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
        pipelineStateStream.pRootSignature = m_RootSignature.GetRootSignature().Get();
        pipelineStateStream.InputLayout = { core::PosNormTexExtendedVertex::InputElementsExtended, core::PosNormTexExtendedVertex::InputElementCountExtended };
        pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pipelineStateStream.DepthStencilState = depthDesc;
        pipelineStateStream.SampleMask = UINT_MAX;
        pipelineStateStream.VS = { g_VoxelGridFill_VS, sizeof(g_VoxelGridFill_VS) };
        pipelineStateStream.GS = { g_VoxelGridFill_GS, sizeof(g_VoxelGridFill_GS) };
        pipelineStateStream.PS = { g_VoxelGridFill_PS, sizeof(g_VoxelGridFill_PS) };
        pipelineStateStream.DSVFormat = DXGI_FORMAT_UNKNOWN;
        pipelineStateStream.RTVFormats[0] = RTFormat;
        pipelineStateStream.RasterizerState = rasterizerDesc;
        pipelineStateStream.BlendState.RenderTarget[0] = blendDesc;
        pipelineStateStream.NumRenderTargets = 1;
        pipelineStateStream.SampleDesc.Count = 1;
        pipelineStateStream.SampleDesc.Quality = 0;

        ThrowIfFailed(device->CreateGraphicsPipelineState(&pipelineStateStream, IID_PPV_ARGS(&m_PipelineState)));
    }

    {
        m_GridParams.gridCellSizes.x = m_GridHalfExtent / (m_GridNum * 0.5f);
        m_GridParams.gridCellSizes.y = 1.0f / m_GridParams.gridCellSizes.x;

        m_GridProjMatrix = XMMatrixOrthographicLH(2.0 * m_GridHalfExtent, 2.0 * m_GridHalfExtent, 0.f, 2.0 * m_GridHalfExtent);
    }
}

void VoxelGrid::InitVoxelGrid(std::shared_ptr<CommandList>& commandList)
{
    // 64 x 64 x 64
    std::vector<VoxelOfVoxelGridClass> voxels(m_GridNum * m_GridNum * m_GridNum, VoxelOfVoxelGridClass{});
    commandList->CopyStructuredBuffer(m_VoxelGrid, voxels);
}

void VoxelGrid::UpdateGrid(std::shared_ptr<CommandList>& commandList, Camera& camera)
{
    commandList->SetScissorRect(m_ScissorRect);
    commandList->SetViewport(m_RenderTarget.GetViewport());
    commandList->SetRenderTarget(m_RenderTarget);
    commandList->SetPipelineState(m_PipelineState);
    commandList->SetGraphicsRootSignature(m_RootSignature);

    {
        m_GridParams.lastSnappedGridPositions = m_GridParams.snappedGridPositions;
        //m_GridParams.gridPositions = camera.GetPosition() +(-camera.GetDirection() * 0.5f * m_GridHalfExtent);
        m_GridParams.gridPositions = { 50, 2, -7, 1};
        //m_GridParams.gridPositions = XMVECTOR{ 50, 2, -7, 1 } + camera.GetDirection() * 0.5f * m_GridHalfExtent;

        m_GridParams.snappedGridPositions = m_GridParams.gridPositions;
        m_GridParams.snappedGridPositions *= m_GridParams.gridCellSizes.y;
        m_GridParams.snappedGridPositions = XMVectorFloor(m_GridParams.snappedGridPositions);
        m_GridParams.snappedGridPositions *= m_GridParams.gridCellSizes.x;
        m_GridParams.snappedGridPositions = DirectX::XMVectorSetW(m_GridParams.snappedGridPositions, 0.0f);

        XMVECTOR offset, translate;
        XMMATRIX translationM, rotationM;
        // back to front viewProjMatrix
        {
            offset = { 0.0, 0.0, m_GridHalfExtent};
            translate = m_GridParams.snappedGridPositions + offset;
            translationM = XMMatrixTranslationFromVector(-translate);
            m_GridParams.gridViewProjMatrices[0] = translationM * m_GridProjMatrix;
        }

        // right to left viewProjMatrix
        {
            offset = { m_GridHalfExtent, 0.0, 0.0};
            translate = m_GridParams.snappedGridPositions + offset;
            rotationM = XMMatrixRotationY(-90.0f);
            translationM = XMMatrixTranslationFromVector(-translate);
            m_GridParams.gridViewProjMatrices[1] = rotationM * translationM * m_GridProjMatrix;
            //m_GridParams.gridViewProjMatrices[1] = translationM * rotationM * m_GridProjMatrix;
        }

        // top to down viewProjMatrix
        {
            offset = { 0.0, m_GridHalfExtent, 0.0};
            translate = m_GridParams.snappedGridPositions + offset;
            rotationM = XMMatrixRotationX(90.0f);
            translationM = XMMatrixTranslationFromVector(-translate);
            m_GridParams.gridViewProjMatrices[2] = rotationM * translationM * m_GridProjMatrix;
            //m_GridParams.gridViewProjMatrices[2] = translationM * rotationM * m_GridProjMatrix;
        }
    }

    commandList->SetGraphicsDynamicConstantBuffer(UpdateParams::b1GridParamsCB, m_GridParams);
    commandList->SetUnorderedAccessView(UpdateParams::u0VoxelsGridSB, 0, m_VoxelGrid);
}

void VoxelGrid::AttachAmbientTex(std::shared_ptr<CommandList>& commandList, const Texture& tex)
{
    commandList->SetShaderResourceView(UpdateParams::t0AmbientTexture, 0, tex);
}

void VoxelGrid::AttachModelMatrix(std::shared_ptr<CommandList>& commandList, const DirectX::XMMATRIX& model)
{
    m_Mat.model = model;
    commandList->SetGraphicsDynamicConstantBuffer(UpdateParams::b0MatCB, m_Mat);
}

const VoxelGridParams& VoxelGrid::GetGridParams() const
{
    return m_GridParams;
}

const StructuredBuffer& VoxelGrid::GetVoxelGrid() const
{
    return m_VoxelGrid;
}