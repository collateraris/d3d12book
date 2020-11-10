#pragma once

#include <URootObject.h>
#include <DirectXMath.h>

namespace dx12demo::core
{
    // When performing transformations on the camera, 
    // it is sometimes useful to express which space this 
    // transformation should be applied.
    enum class ESpace
    {
        Local,
        World,
    };

    class Camera : public URootObject
    {
    public:

        Camera();
        virtual ~Camera();

        void XM_CALLCONV set_LookAt(const DirectX::FXMVECTOR& eye, const DirectX::FXMVECTOR& target, const DirectX::FXMVECTOR& up);
        const DirectX::XMMATRIX& get_ViewMatrix() const;
        const DirectX::XMMATRIX& get_InverseViewMatrix() const;

        void set_Projection(float fovy, float aspect, float zNear, float zFar);
        const DirectX::XMMATRIX& get_ProjectionMatrix() const;
        const DirectX::XMMATRIX& get_InverseProjectionMatrix() const;

        const DirectX::XMVECTOR& GetPosition() const;
        const DirectX::XMVECTOR& GetDirection() const;

        void set_FoV(float fovy);
        float get_FoV() const;

        /**
         * Set the camera's position in world-space.
         */
        void XM_CALLCONV set_Translation(const DirectX::FXMVECTOR& translation);
        const DirectX::XMVECTOR& get_Translation() const;

        void XM_CALLCONV set_Rotation(const DirectX::FXMVECTOR& rotation);
        const DirectX::FXMVECTOR& get_Rotation() const;

        void XM_CALLCONV Translate(const DirectX::FXMVECTOR& translation, ESpace space = ESpace::Local);
        void Rotate(const DirectX::FXMVECTOR& quaternion);

    protected:
        virtual void UpdateViewMatrix() const;
        virtual void UpdateInverseViewMatrix() const;
        virtual void UpdateProjectionMatrix() const;
        virtual void UpdateInverseProjectionMatrix() const;

        // This data must be aligned otherwise the SSE intrinsics fail
        // and throw exceptions.
        __declspec(align(16)) struct AlignedData
        {
            // World-space position of the camera.
            DirectX::XMVECTOR m_Translation;
            // World-space rotation of the camera.
            // THIS IS A QUATERNION!!!!
            DirectX::XMVECTOR m_Rotation;

            DirectX::XMMATRIX m_ViewMatrix, m_InverseViewMatrix;
            DirectX::XMMATRIX m_ProjectionMatrix, m_InverseProjectionMatrix;
        };
        AlignedData* pData;

        // projection parameters
        float m_vFoV;   // Vertical field of view.
        float m_AspectRatio; // Aspect ratio
        float m_zNear;      // Near clip distance
        float m_zFar;       // Far clip distance.

        // True if the view matrix needs to be updated.
        mutable bool m_ViewDirty, m_InverseViewDirty;
        // True if the projection matrix needs to be updated.
        mutable bool m_ProjectionDirty, m_InverseProjectionDirty;
    };
}