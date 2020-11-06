#pragma once

#include <GeometryPrimitive.h>

namespace dx12demo::core
{
    __declspec(align(16)) struct DispatchParams
    {
        DirectX::XMFLOAT3 m_NumThreadGroups;
        DirectX::XMFLOAT3 m_NumThreads;
    };


    __declspec(align(16)) struct ScreenToViewParams
    {
        DirectX::XMMATRIX m_InverseProjectionMatrix;
        DirectX::XMFLOAT2 m_ScreenDimensions;
    };
}