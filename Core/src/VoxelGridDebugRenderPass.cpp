#include <VoxelGridDebugRenderPass.h>

#include <DX12LibPCH.h>
#include <Application.h>
#include <CommandList.h>
#include <VoxelGridFill_VS.h>
#include <VoxelGridDebug_PS.h>
#include <Mesh.h>

using namespace dx12demo::core;

namespace DebugParams
{
    enum
    {
        b0MatCB,
        b1GridParamsCB,
        t0VoxelsGridSB,
        NumRootParameters
    };
}

VoxelGridDebugRenderPass::VoxelGridDebugRenderPass()
{

}

VoxelGridDebugRenderPass::~VoxelGridDebugRenderPass()
{

}

void VoxelGridDebugRenderPass::LoadContent(RenderPassBaseInfo* info)
{
    VoxelGridDebugRenderPassInfo* debugInfo = dynamic_cast<VoxelGridDebugRenderPassInfo*>(info);
    //debug grid 
    {
        CD3DX12_ROOT_PARAMETER1 rootParameters[DebugParams::NumRootParameters];
        rootParameters[DebugParams::b0MatCB].InitAsConstantBufferView(0, 0);
        rootParameters[DebugParams::b1GridParamsCB].InitAsConstantBufferView(1, 0);
        CD3DX12_DESCRIPTOR_RANGE1 voxelsGridDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
        rootParameters[DebugParams::t0VoxelsGridSB].InitAsDescriptorTable(1, &voxelsGridDescrRange);

        // Allow input layout and deny unnecessary access to certain pipeline stages.
        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
        rootSignatureDescription.Init_1_1(DebugParams::NumRootParameters, rootParameters, 0, nullptr, rootSignatureFlags);

        m_DebugRootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, debugInfo->rootSignatureVersion);

        CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
        rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

        struct PipelineStateStream
        {
            CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
            CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
            CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
            CD3DX12_PIPELINE_STATE_STREAM_VS VS;
            CD3DX12_PIPELINE_STATE_STREAM_PS PS;
            CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER Rasterizer;
            CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
            CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
        } pipelineStateStream;

        pipelineStateStream.pRootSignature = m_DebugRootSignature.GetRootSignature().Get();
        pipelineStateStream.InputLayout = { core::PosNormTexExtendedVertex::InputElementsExtended, core::PosNormTexExtendedVertex::InputElementCountExtended };
        pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pipelineStateStream.VS = { g_VoxelGridFill_VS, sizeof(g_VoxelGridFill_VS) };
        pipelineStateStream.PS = { g_VoxelGridDebug_PS, sizeof(g_VoxelGridDebug_PS) };
        pipelineStateStream.Rasterizer = rasterizerDesc;
        pipelineStateStream.RTVFormats = debugInfo->rtvFormats;
        pipelineStateStream.DSVFormat = debugInfo->depthBufferFormat;

        D3D12_PIPELINE_STATE_STREAM_DESC PipelineStateStreamDesc = {
            sizeof(PipelineStateStream), &pipelineStateStream
        };

        auto& device = GetApp().GetDevice();
        ThrowIfFailed(device->CreatePipelineState(&PipelineStateStreamDesc, IID_PPV_ARGS(&m_DebugPipelineState)));
    }
}

void VoxelGridDebugRenderPass::OnUpdate(std::shared_ptr<CommandList>& commandList, UpdateEventArgs& e)
{

}

void VoxelGridDebugRenderPass::OnPreRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e)
{
    commandList->SetPipelineState(m_DebugPipelineState);
    commandList->SetGraphicsRootSignature(m_DebugRootSignature);
}

void VoxelGridDebugRenderPass::OnRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e)
{

}

void VoxelGridDebugRenderPass::AttachVoxelGrid(std::shared_ptr<CommandList>& commandList, const StructuredBuffer& sb)
{
    commandList->SetShaderResourceView(DebugParams::t0VoxelsGridSB, 0, sb, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

void VoxelGridDebugRenderPass::AttachVoxelGridParams(std::shared_ptr<CommandList>& commandList, const VoxelGrid& voxelGrid)
{
    commandList->SetGraphicsDynamicConstantBuffer(DebugParams::b1GridParamsCB, voxelGrid.GetGridParams());
}

void VoxelGridDebugRenderPass::AttachModelViewOrtoProjMatrix(std::shared_ptr<CommandList>& commandList, const DirectX::XMMATRIX& model, const DirectX::XMMATRIX& modelViewOrtoProj)
{
    m_Mat.model = model;
    m_Mat.modelViewOrtoProjMatrix = modelViewOrtoProj;
    commandList->SetGraphicsDynamicConstantBuffer(DebugParams::b0MatCB, m_Mat);
}