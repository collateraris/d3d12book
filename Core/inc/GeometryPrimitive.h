#pragma once

#include <DirectXMath.h>

namespace dx12demo::core
{
    __declspec(align(16)) struct Plane
    {
        DirectX::XMFLOAT3 N;   // Plane normal.
        float  d;   // Distance to origin.
    };

    __declspec(align(16)) struct Sphere
    {
        DirectX::XMFLOAT3 c;   // Center point.
        float  r;   // Radius.
    };

    __declspec(align(16)) struct Cone
    {
        DirectX::XMFLOAT3 T;   // Cone tip.
        DirectX::XMFLOAT3 d;   // Direction of the cone.
        float  h;   // Height of the cone.
        float  r;   // bottom radius of the cone.
    };

    // Four planes of a view frustum (in view space).
    // The planes are:
    //  * Left,
    //  * Right,
    //  * Top,
    //  * Bottom.
    // The back and/or front planes can be computed from depth values in the 
    // light culling compute shader.
    __declspec(align(16)) struct FrustumPrimitive
    {
        Plane planes[4];   // left, right, top, bottom frustum planes.
    };
}