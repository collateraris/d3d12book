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
        b0GridParamsCB,
        t0AmbientTexture,
        u0VoxelsGridSB,
        NumRootParameters
    };
}

VoxelGrid::VoxelGrid()
{
    auto device = GetApp().GetDevice();

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    //update grid 
    {
        CD3DX12_ROOT_PARAMETER1 rootParameters[UpdateParams::NumRootParameters];
        rootParameters[UpdateParams::b0GridParamsCB].InitAsConstantBufferView(0, 0);
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

        CD3DX12_DEPTH_STENCIL_DESC depthDesc(D3D12_DEFAULT);
        depthDesc.DepthEnable = false;

        CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);

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
        pipelineStateStream.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
        pipelineStateStream.RasterizerState = rasterizerDesc;
        pipelineStateStream.NumRenderTargets = 0;
        pipelineStateStream.SampleDesc.Count = 1;
        pipelineStateStream.SampleDesc.Quality = 0;

        ThrowIfFailed(device->CreateGraphicsPipelineState(&pipelineStateStream, IID_PPV_ARGS(&m_PipelineState)));
    }

    {
        m_GridParams.gridCellSizes.x = m_GridHalfExtent / 16.0f;
        m_GridParams.gridCellSizes.y = 1.0f / m_GridParams.gridCellSizes.x;

        m_GridProjMatrix = XMMatrixOrthographicLH(2.0 * m_GridHalfExtent, 2.0 * m_GridHalfExtent, 0.f, 2.0 * m_GridHalfExtent);
    }
}

VoxelGrid::~VoxelGrid()
{

}

void VoxelGrid::InitVoxelGrid(std::shared_ptr<CommandList>& commandList)
{
    // 32 x 32 x 32
    std::vector<VoxelOfVoxelGridClass> voxels(32 * 32 * 32, VoxelOfVoxelGridClass{});
    commandList->CopyStructuredBuffer(m_VoxelGrid, voxels);
}

void VoxelGrid::UpdateGrid(std::shared_ptr<CommandList>& commandList, Camera& camera)
{
    commandList->SetPipelineState(m_PipelineState);
    commandList->SetGraphicsRootSignature(m_RootSignature);

    {
        m_GridParams.lastSnappedGridPositions = m_GridParams.snappedGridPositions;
        m_GridParams.gridPositions = camera.GetPosition() + (camera.GetDirection() * 0.5f * m_GridHalfExtent);
        m_GridParams.snappedGridPositions = m_GridParams.gridPositions;
        m_GridParams.snappedGridPositions *= m_GridParams.gridCellSizes.x;
        m_GridParams.snappedGridPositions = XMVectorFloor(m_GridParams.snappedGridPositions);
        m_GridParams.snappedGridPositions *= m_GridParams.gridCellSizes.y;

        XMVECTOR offset, translate;
        XMMATRIX translationM, rotationM;
        // back to front viewProjMatrix
        {
            offset = { 0.0, 0.0, m_GridHalfExtent, 1.0 };
            translate = -m_GridParams.snappedGridPositions - offset;
            translationM = XMMatrixTranslationFromVector(translate);
            m_GridParams.gridViewProjMatrices[0] = translationM * m_GridProjMatrix;
        }

        // right to left viewProjMatrix
        {
            offset = { m_GridHalfExtent, 0.0, 0.0, 1.0 };
            translate = -m_GridParams.snappedGridPositions - offset;
            rotationM = XMMatrixRotationY(-90.0f);
            translationM = XMMatrixTranslationFromVector(translate);
            m_GridParams.gridViewProjMatrices[1] = rotationM * translationM * m_GridProjMatrix;
        }

        // top to down viewProjMatrix
        {
            offset = { 0.0, m_GridHalfExtent, 0.0, 1.0 };
            translate = -m_GridParams.snappedGridPositions - offset;
            rotationM = XMMatrixRotationX(90.0f);
            translationM = XMMatrixTranslationFromVector(translate);
            m_GridParams.gridViewProjMatrices[2] = rotationM * translationM * m_GridProjMatrix;
        }
    }

    commandList->SetGraphicsDynamicConstantBuffer(UpdateParams::b0GridParamsCB, m_GridParams);
    commandList->SetUnorderedAccessView(UpdateParams::u0VoxelsGridSB, 0, m_VoxelGrid);
}

void VoxelGrid::AttachAmbientTex(std::shared_ptr<CommandList>& commandList, const Texture& tex)
{
    commandList->SetShaderResourceView(UpdateParams::t0AmbientTexture, 0, tex);
}

const VoxelGridParams& VoxelGrid::GetGridParams() const
{
    return m_GridParams;
}

const StructuredBuffer& VoxelGrid::GetVoxelGrid() const
{
    return m_VoxelGrid;
}