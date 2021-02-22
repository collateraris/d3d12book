#include <AtmosphericScatteringSkybox.h>

#include <DX12LibPCH.h>

#include <AtmosphericScatteringSkybox_PS.h>
#include <Skybox_VS.h>

#include <Application.h>
#include <CommandQueue.h>

using namespace dx12demo::core;
using namespace DirectX;

namespace ShaderParams
{
    enum
    {
        b0MatCB,
        t0RS_SkyboxLUT3DTex,
        t1MS_SkyboxLUT3DTex,
        t2AtmDataSB,
        t3ExtDataSB,
        NumRootParameters,
    };
}

AtmosphericScatteringParamsWrapperSB::AtmosphericScatteringParamsWrapperSB()
{

}

AtmosphericScatteringParamsWrapperSB::~AtmosphericScatteringParamsWrapperSB()
{

}

bool AtmosphericScatteringParamsWrapperSB::IsNeedUpdateAtmParams() const
{
    return bIsNeedUpdate;
}

void AtmosphericScatteringParamsWrapperSB::UpdateAtmParams(std::shared_ptr<CommandList>& commandList)
{
    m_Params._ScatteringR = { m_RayleighSct.x * m_RayleighScatterCoef, m_RayleighSct.y * m_RayleighScatterCoef, m_RayleighSct.z * m_RayleighScatterCoef, m_RayleighSct.w * m_RayleighScatterCoef };
    m_Params._ScatteringM = { m_MieSct.x * m_MieScatterCoef, m_MieSct.y * m_MieScatterCoef, m_MieSct.z * m_MieScatterCoef, m_MieSct.w * m_MieScatterCoef };

    m_Params._ExtinctionR = { m_RayleighSct.x * m_RayleighExtinctionCoef, m_RayleighSct.y * m_RayleighExtinctionCoef, m_RayleighSct.z * m_RayleighExtinctionCoef, m_RayleighSct.w * m_RayleighExtinctionCoef };
    m_Params._ExtinctionM = { m_MieSct.x * m_MieExtinctionCoef, m_MieSct.y * m_MieExtinctionCoef, m_MieSct.z * m_MieExtinctionCoef, m_MieSct.w * m_MieExtinctionCoef };

    commandList->CopyStructuredBuffer(m_ParamsSB, m_Params);

    bIsNeedUpdate = false;
}

const StructuredBuffer& AtmosphericScatteringParamsWrapperSB::GetAtmParamsSB() const
{
    return m_ParamsSB;
}

void AtmosphericScatteringParamsWrapperSB::Set_AtmosphereHeight(float p)
{
    m_Params._AtmosphereHeight = p;

    bIsNeedUpdate = true;
}

void AtmosphericScatteringParamsWrapperSB::Set_PlanetRadius(float p)
{
    m_Params._PlanetRadius = p;

    bIsNeedUpdate = true;
}

void AtmosphericScatteringParamsWrapperSB::Set_MieG(float p)
{
    m_Params._MieG = p;

    bIsNeedUpdate = true;
}

void AtmosphericScatteringParamsWrapperSB::Set_DistanceScale(float p)
{
    m_Params._DistanceScale = p;

    bIsNeedUpdate = true;
}

void AtmosphericScatteringParamsWrapperSB::Set__DensityScaleHeight(const DirectX::XMFLOAT2& p)
{
    m_Params._DensityScaleHeight = p;

    bIsNeedUpdate = true;
}

void AtmosphericScatteringParamsWrapperSB::Set__SunIntensity(float p)
{
    m_Params._SunIntensity = p;

    bIsNeedUpdate = true;
} 

void AtmosphericScatteringParamsWrapperSB::SetRayleighScatterCoef(float p)
{
    m_RayleighScatterCoef = std::clamp(p, 0.f, 10.f);

    bIsNeedUpdate = true;
}

void AtmosphericScatteringParamsWrapperSB::SetRayleighExtinctionCoef(float p)
{
    m_RayleighExtinctionCoef = std::clamp(p, 0.f, 10.f);

    bIsNeedUpdate = true;
}

void AtmosphericScatteringParamsWrapperSB::SetMieScatterCoef(float p)
{
    m_MieScatterCoef = std::clamp(p, 0.f, 10.f);

    bIsNeedUpdate = true;
}

void AtmosphericScatteringParamsWrapperSB::SetMieExtinctionCoef(float p)
{
    m_MieExtinctionCoef = std::clamp(p, 0.f, 10.f);

    bIsNeedUpdate = true;
}

AtmosphericScatteringSkyboxRP::AtmosphericScatteringSkyboxRP()
{

}

AtmosphericScatteringSkyboxRP::~AtmosphericScatteringSkyboxRP()
{

}

void AtmosphericScatteringSkyboxRP::LoadContent(RenderPassBaseInfo* info)
{

    AtmosphericScatteringSkyboxRPInfo* envInfo = dynamic_cast<AtmosphericScatteringSkyboxRPInfo*>(info);

    PrecomputeParticleDensityForAtmScatRenderPassInfo ppdrpinfo;
    ppdrpinfo.rootSignatureVersion = envInfo->rootSignatureVersion;
    m_PrecomputeParticleDensityRP.LoadContent(&ppdrpinfo);

    m_Camera = envInfo->camera;
	m_SkyboxMesh = core::Mesh::CreateCube(*envInfo->commandList, 1.0f, true);

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

        CD3DX12_DESCRIPTOR_RANGE1 rs_SkyboxLUT3DTexDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
        CD3DX12_DESCRIPTOR_RANGE1 ms_SkyboxLUT3DTexDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
        CD3DX12_DESCRIPTOR_RANGE1 atmDataDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);
        CD3DX12_DESCRIPTOR_RANGE1 externalDataDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3);
        CD3DX12_ROOT_PARAMETER1 rootParameters[ShaderParams::NumRootParameters];
        rootParameters[ShaderParams::b0MatCB].InitAsConstants(sizeof(DirectX::XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        rootParameters[ShaderParams::t0RS_SkyboxLUT3DTex].InitAsDescriptorTable(1, &rs_SkyboxLUT3DTexDescriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[ShaderParams::t1MS_SkyboxLUT3DTex].InitAsDescriptorTable(1, &ms_SkyboxLUT3DTexDescriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[ShaderParams::t2AtmDataSB].InitAsDescriptorTable(1, &atmDataDescriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[ShaderParams::t3ExtDataSB].InitAsDescriptorTable(1, &externalDataDescriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

        CD3DX12_STATIC_SAMPLER_DESC linearClampSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
        rootSignatureDescription.Init_1_1(ShaderParams::NumRootParameters, rootParameters, 1, &linearClampSampler, rootSignatureFlags);

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
        skyboxPipelineStateStream.PS = { g_AtmosphericScatteringSkybox_PS, sizeof(g_AtmosphericScatteringSkybox_PS) };
        skyboxPipelineStateStream.RTVFormats = envInfo->rtvFormats;

        D3D12_PIPELINE_STATE_STREAM_DESC skyboxPipelineStateStreamDesc = {
            sizeof(SkyboxPipelineState), &skyboxPipelineStateStream
        };

        auto& device = GetApp().GetDevice();
        ThrowIfFailed(device->CreatePipelineState(&skyboxPipelineStateStreamDesc, IID_PPV_ARGS(&m_SkyboxPipelineState)));
    }
}

void AtmosphericScatteringSkyboxRP::OnUpdate(std::shared_ptr<CommandList>& commandList, UpdateEventArgs& e)
{
    auto viewMatrix = XMMatrixTranspose(XMMatrixRotationQuaternion(m_Camera->get_Rotation()));
    auto& projMatrix = m_Camera->get_ProjectionMatrix();
    m_ViewProjMatrix = viewMatrix * projMatrix;

    const auto& camPos = m_Camera->GetPosition();
    XMStoreFloat3(&m_ExtData.camPos, camPos);
}

void AtmosphericScatteringSkyboxRP::OnRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e)
{
    commandList->CopyStructuredBuffer(m_ExtDataSB, m_ExtData);

    if (IsNeedUpdateAtmParams())
    {
        UpdateAtmParams(commandList);

        m_PrecomputeSkyboxLUTCRP.StartCompute(commandList);
        m_PrecomputeSkyboxLUTCRP.BindPrecomputeParticleDensity(commandList, m_PrecomputeParticleDensityRP.GetPrecomputeParticleDensity());
        m_PrecomputeSkyboxLUTCRP.BindAtmosphereParams(commandList, GetAtmParamsSB());
        m_PrecomputeSkyboxLUTCRP.Compute(commandList);
    }

    commandList->SetPipelineState(m_SkyboxPipelineState);
    commandList->SetGraphicsRootSignature(m_SkyboxSignature);

    commandList->SetGraphics32BitConstants(ShaderParams::b0MatCB, m_ViewProjMatrix);
    commandList->SetShaderResourceView(ShaderParams::t0RS_SkyboxLUT3DTex, 0, m_PrecomputeSkyboxLUTCRP.Get_RS_SkyboxLUT3DTex());
    commandList->SetShaderResourceView(ShaderParams::t1MS_SkyboxLUT3DTex, 0, m_PrecomputeSkyboxLUTCRP.Get_MS_SkyboxLUT3DTex());
    commandList->SetShaderResourceView(ShaderParams::t2AtmDataSB, 0, GetAtmParamsSB());
    commandList->SetShaderResourceView(ShaderParams::t3ExtDataSB, 0, m_ExtDataSB);

    m_SkyboxMesh->Render(commandList);
}

void AtmosphericScatteringSkyboxRP::BindLightPos(const DirectX::XMFLOAT3& lightPos)
{
    m_ExtData.lightPos = lightPos;
}