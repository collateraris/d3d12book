#include <PrecomputeSkyboxLUTForAtmScat.h>

#include <DX12LibPCH.h>

#include <PrecomputeAtmosphericScatteringSkybox_CS.h>
#include <Application.h>
#include <CommandQueue.h>

using namespace dx12demo::core;

namespace ComputeParams
{
    enum
    {
        t0PrecomputeParticleDensityTex,
        t1AtmParSB,
        u0RS_SkyboxLUT3DTex,
        u1MS_SkyboxLUT3DTex,
        NumRootParameters
    };
}

PrecomputeSkyboxLUTForAtmScat::PrecomputeSkyboxLUTForAtmScat()
{
    auto device = GetApp().GetDevice();

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    CD3DX12_ROOT_PARAMETER1 rootParameters[ComputeParams::NumRootParameters];

    CD3DX12_DESCRIPTOR_RANGE1 precomputeParticleDensityTexDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    rootParameters[ComputeParams::t0PrecomputeParticleDensityTex].InitAsDescriptorTable(1, &precomputeParticleDensityTexDescrRange);
    CD3DX12_DESCRIPTOR_RANGE1 atmParSBDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
    rootParameters[ComputeParams::t1AtmParSB].InitAsDescriptorTable(1, &atmParSBDescrRange);

    CD3DX12_DESCRIPTOR_RANGE1 rs_SkyboxLUT3DTexDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
    rootParameters[ComputeParams::u0RS_SkyboxLUT3DTex].InitAsDescriptorTable(1, &rs_SkyboxLUT3DTexDescrRange);
    CD3DX12_DESCRIPTOR_RANGE1 ms_SkyboxLUT3DTexDescrRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);
    rootParameters[ComputeParams::u1MS_SkyboxLUT3DTex].InitAsDescriptorTable(1, &ms_SkyboxLUT3DTexDescrRange);

    const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
        0, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(ComputeParams::NumRootParameters, rootParameters, 1, &linearClamp, D3D12_ROOT_SIGNATURE_FLAG_NONE);

    m_RootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

    // Create the PSO for GenerateMips shader.
    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_CS CS;
    } pipelineStateStream;

    pipelineStateStream.pRootSignature = m_RootSignature.GetRootSignature().Get();
    pipelineStateStream.CS = { g_PrecomputeAtmosphericScatteringSkybox_CS, sizeof(g_PrecomputeAtmosphericScatteringSkybox_CS) };

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
       sizeof(PipelineStateStream), &pipelineStateStream
    };

    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState)));

    {
        DXGI_FORMAT format = DXGI_FORMAT_R16G16B16A16_FLOAT;

        auto colorDesc = CD3DX12_RESOURCE_DESC::Tex3D(format, 32, 128, 32);
        colorDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        m_RS_SkyboxLUT = core::Texture(colorDesc, nullptr,
            TextureUsage::Albedo,
            L"m_RS_SkyboxLUT Texture");
    }

    {
        DXGI_FORMAT format = DXGI_FORMAT_R16G16_FLOAT;

        auto colorDesc = CD3DX12_RESOURCE_DESC::Tex3D(format, 32, 128, 32);
        colorDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        m_MS_SkyboxLUT = core::Texture(colorDesc, nullptr,
            TextureUsage::Albedo,
            L"m_MS_SkyboxLUT Texture");
    }
}

PrecomputeSkyboxLUTForAtmScat::~PrecomputeSkyboxLUTForAtmScat()
{

}

void PrecomputeSkyboxLUTForAtmScat::StartCompute(std::shared_ptr<CommandList>& commandList)
{
    commandList->SetComputeRootSignature(m_RootSignature);
    commandList->SetPipelineState(m_PipelineState);

    m_CurrState.set(EBitsetStates::bStartCompute);
}


void PrecomputeSkyboxLUTForAtmScat::BindPrecomputeParticleDensity(std::shared_ptr<CommandList>& commandList, const Texture& partcleDens)
{
    assert(m_CurrState.test(EBitsetStates::bStartCompute));

    commandList->SetShaderResourceView(ComputeParams::t0PrecomputeParticleDensityTex, 0, partcleDens);

    m_CurrState.set(EBitsetStates::bBindPrecomputeParticleDensity);
}

void PrecomputeSkyboxLUTForAtmScat::BindAtmosphereParams(std::shared_ptr<CommandList>& commandList, const StructuredBuffer& atmPar)
{
    assert(m_CurrState.test(EBitsetStates::bStartCompute));

    commandList->SetShaderResourceView(ComputeParams::t1AtmParSB, 0, atmPar);

    m_CurrState.set(EBitsetStates::bBindAtmPar);
}

void PrecomputeSkyboxLUTForAtmScat::Compute(std::shared_ptr<CommandList>& commandList)
{
    assert(m_CurrState.test(EBitsetStates::bStartCompute));
    assert(m_CurrState.test(EBitsetStates::bBindPrecomputeParticleDensity));
    assert(m_CurrState.test(EBitsetStates::bBindAtmPar));

    m_CurrState.reset(EBitsetStates::bStartCompute);
    m_CurrState.reset(EBitsetStates::bBindAtmPar);
    m_CurrState.reset(EBitsetStates::bBindPrecomputeParticleDensity);

    commandList->SetUnorderedAccessView(ComputeParams::u0RS_SkyboxLUT3DTex, 0, m_RS_SkyboxLUT);
    commandList->SetUnorderedAccessView(ComputeParams::u1MS_SkyboxLUT3DTex, 0, m_MS_SkyboxLUT);

    commandList->Dispatch(32, 128, 32);
}

const Texture& PrecomputeSkyboxLUTForAtmScat::Get_RS_SkyboxLUT3DTex() const
{
    return m_RS_SkyboxLUT;
}

const Texture& PrecomputeSkyboxLUTForAtmScat::Get_MS_SkyboxLUT3DTex() const
{
    return m_MS_SkyboxLUT;
}
