#pragma once

#include <DirectXMath.h>

namespace dx12demo::core
{
    __declspec(align(16)) struct PointLight
    {
        PointLight()
            : PositionWS(0.0f, 0.0f, 0.0f, 1.0f)
            , PositionVS(0.0f, 0.0f, 0.0f, 1.0f)
            , Color(1.0f, 1.0f, 1.0f, 1.0f)
            , Intensity(1.0f)
            , Attenuation(0.0f)
        {}

        DirectX::XMFLOAT4    PositionWS; // Light position in world space.
        //----------------------------------- (16 byte boundary)
        DirectX::XMFLOAT4    PositionVS; // Light position in view space.
        //----------------------------------- (16 byte boundary)
        DirectX::XMFLOAT4    Color;
        //----------------------------------- (16 byte boundary)
        float       Intensity;
        float       Attenuation;
        float       Padding[2];             // Pad to 16 bytes.
        //----------------------------------- (16 byte boundary)
        // Total:                              16 * 4 = 64 bytes
    };

    __declspec(align(16)) struct SpotLight
    {
        SpotLight()
            : PositionWS(0.0f, 0.0f, 0.0f, 1.0f)
            , PositionVS(0.0f, 0.0f, 0.0f, 1.0f)
            , DirectionWS(0.0f, 0.0f, 1.0f, 0.0f)
            , DirectionVS(0.0f, 0.0f, 1.0f, 0.0f)
            , Color(1.0f, 1.0f, 1.0f, 1.0f)
            , Intensity(1.0f)
            , SpotAngle(DirectX::XM_PIDIV2)
            , Attenuation(0.0f)
        {}

        DirectX::XMFLOAT4    PositionWS; // Light position in world space.
        //----------------------------------- (16 byte boundary)
        DirectX::XMFLOAT4    PositionVS; // Light position in view space.
        //----------------------------------- (16 byte boundary)
        DirectX::XMFLOAT4    DirectionWS; // Light direction in world space.
        //----------------------------------- (16 byte boundary)
        DirectX::XMFLOAT4    DirectionVS; // Light direction in view space.
        //----------------------------------- (16 byte boundary)
        DirectX::XMFLOAT4    Color;
        //----------------------------------- (16 byte boundary)
        float       Intensity;
        float       SpotAngle;
        float       Attenuation;
        float       Padding;                // Pad to 16 bytes.
        //----------------------------------- (16 byte boundary)
        // Total:                              16 * 6 = 96 bytes
    };

    __declspec(align(16)) struct DirectionalLight
    {
        DirectionalLight()
            : DirectionWS(0.0f, 0.0f, 1.0f, 0.0f)
            , DirectionVS(0.0f, 0.0f, 1.0f, 0.0f)
            , Color(1.0f, 1.0f, 1.0f, 1.0f)
        {}

        DirectX::XMFLOAT4    DirectionWS; // Light direction in world space.
        //----------------------------------- (16 byte boundary)
        DirectX::XMFLOAT4    DirectionVS; // Light direction in view space.
        //----------------------------------- (16 byte boundary)
        DirectX::XMFLOAT4    Color;
        //----------------------------------- (16 byte boundary)
        // Total:                              16 * 3 = 48 bytes 
    };

    __declspec(align(16))
    struct Light
    {
        /**
        * Position for point and spot lights (World space).
        */
        DirectX::XMFLOAT4 m_PositionWS;
        //--------------------------------------------------------------( 16 bytes )
        /**
        * Direction for spot and directional lights (World space).
        */
        DirectX::XMFLOAT4   m_DirectionWS;
        //--------------------------------------------------------------( 16 bytes )
        /**
        * Position for point and spot lights (View space).
        */
        DirectX::XMFLOAT4   m_PositionVS;
        //--------------------------------------------------------------( 16 bytes )
        /**
        * Direction for spot and directional lights (View space).
        */
        DirectX::XMFLOAT4   m_DirectionVS;
        //--------------------------------------------------------------( 16 bytes )
        /**
         * Color of the light. Diffuse and specular colors are not separated.
         */
        DirectX::XMFLOAT4   m_Color;
        //--------------------------------------------------------------( 16 bytes )
        /**
         * The half angle of the spotlight cone.
         */
        
        float m_SpotlightAngle;
        /**
         * The range of the light.
         */
        float       m_Range;

        /**
         * The intensity of the light.
         */
        float       m_Intensity;

        float       m_Attenuation;
        //--------------------------------------------------------------(16 bytes )
        /**
         * Disable or enable the light.
         */
        int    m_Enabled;

        /**
         * True if the light is selected in the editor.
         */
        int    m_Selected;
        /**
         * The type of the light.
         */
        int   m_Type;
        int mPadding;
       
        //--------------------------------------------------------------(16 bytes )
        //--------------------------------------------------------------( 16 * 7 = 112 bytes )
        Light::Light()
            : m_PositionWS(0, 0, 0, 1)
            , m_DirectionWS(0, 0, -1, 0)
            , m_PositionVS(0, 0, 0, 1)
            , m_DirectionVS(0, 0, 1, 0)
            , m_Color(1, 1, 1, 1)
            , m_SpotlightAngle(45.0f)
            , m_Range(100.0f)
            , m_Intensity(1.0f)
            , m_Enabled(true)
            , m_Selected(false)
            , m_Type(0)
        {}
    };
}