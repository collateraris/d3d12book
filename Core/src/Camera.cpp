#include <Camera.h>

using namespace dx12demo::core;
using namespace DirectX;

Camera::Camera()
    : m_ViewDirty(true)
    , m_InverseViewDirty(true)
    , m_ProjectionDirty(true)
    , m_InverseProjectionDirty(true)
    , m_vFoV(45.0f)
    , m_AspectRatio(1.0f)
    , m_zNear(0.1f)
    , m_zFar(100.0f)
{
    pData = (AlignedData*)_aligned_malloc(sizeof(AlignedData), 16);
    pData->m_Translation = DirectX::XMVectorZero();
    pData->m_Rotation = DirectX::XMQuaternionIdentity();
}

Camera::~Camera()
{
    _aligned_free(pData);
}

void XM_CALLCONV Camera::set_LookAt(const DirectX::FXMVECTOR& eye, const DirectX::FXMVECTOR& target, const DirectX::FXMVECTOR& up)
{
    pData->m_ViewMatrix = DirectX::XMMatrixLookAtLH(eye, target, up);

    pData->m_Translation = eye;
    pData->m_Rotation = DirectX::XMQuaternionRotationMatrix(XMMatrixTranspose(pData->m_ViewMatrix));

    m_InverseViewDirty = true;
    m_ViewDirty = false;
}

const DirectX::XMMATRIX& Camera::get_ViewMatrix() const
{
    if (m_ViewDirty)
    {
        UpdateViewMatrix();
    }
    return pData->m_ViewMatrix;
}

const DirectX::XMMATRIX& Camera::get_InverseViewMatrix() const
{
    if (m_InverseViewDirty)
    {
        pData->m_InverseViewMatrix = DirectX::XMMatrixInverse(nullptr, pData->m_ViewMatrix);
        m_InverseViewDirty = false;
    }

    return pData->m_InverseViewMatrix;
}

void Camera::set_Projection(float fovy, float aspect, float zNear, float zFar)
{
    m_vFoV = fovy;
    m_AspectRatio = aspect;
    m_zNear = zNear;
    m_zFar = zFar;

    m_ProjectionDirty = true;
    m_InverseProjectionDirty = true;
}

const DirectX::XMMATRIX& Camera::get_ProjectionMatrix() const
{
    if (m_ProjectionDirty)
    {
        UpdateProjectionMatrix();
    }

    return pData->m_ProjectionMatrix;
}

const DirectX::XMMATRIX& Camera::get_InverseProjectionMatrix() const
{
    if (m_InverseProjectionDirty)
    {
        UpdateInverseProjectionMatrix();
    }

    return pData->m_InverseProjectionMatrix;
}

void Camera::set_FoV(float fovy)
{
    if (m_vFoV != fovy)
    {
        m_vFoV = fovy;
        m_ProjectionDirty = true;
        m_InverseProjectionDirty = true;
    }
}

float Camera::get_FoV() const
{
    return m_vFoV;
}

void XM_CALLCONV Camera::set_Translation(const DirectX::FXMVECTOR& translation)
{
    pData->m_Translation = translation;
    m_ViewDirty = true;
}

const DirectX::FXMVECTOR& Camera::get_Translation() const
{
    return pData->m_Translation;
}

void Camera::set_Rotation(const DirectX::FXMVECTOR& rotation)
{
    pData->m_Rotation = rotation;
}

const DirectX::FXMVECTOR& Camera::get_Rotation() const
{
    return pData->m_Rotation;
}

void XM_CALLCONV Camera::Translate(const DirectX::FXMVECTOR& translation, ESpace space)
{
    switch (space)
    {
    case ESpace::Local:
    {
        pData->m_Translation += DirectX::XMVector3Rotate(translation, pData->m_Rotation);
    }
    break;
    case ESpace::World:
    {
        pData->m_Translation += translation;
    }
    break;
    }

    pData->m_Translation = DirectX::XMVectorSetW(pData->m_Translation, 1.0f);

    m_ViewDirty = true;
    m_InverseViewDirty = true;
}

void Camera::Rotate(const DirectX::FXMVECTOR& quaternion)
{
    pData->m_Rotation = DirectX::XMQuaternionMultiply(pData->m_Rotation, quaternion);

    m_ViewDirty = true;
    m_InverseViewDirty = true;
}

void Camera::UpdateViewMatrix() const
{
    DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixTranspose(DirectX::XMMatrixRotationQuaternion(pData->m_Rotation));
    DirectX::XMMATRIX translationMatrix = DirectX::XMMatrixTranslationFromVector(-(pData->m_Translation));

    pData->m_ViewMatrix = translationMatrix * rotationMatrix;

    m_InverseViewDirty = true;
    m_ViewDirty = false;
}

void Camera::UpdateInverseViewMatrix() const
{
    if (m_ViewDirty)
    {
        UpdateViewMatrix();
    }

    pData->m_InverseViewMatrix = DirectX::XMMatrixInverse(nullptr, pData->m_ViewMatrix);
    m_InverseViewDirty = false;
}

void Camera::UpdateProjectionMatrix() const
{
    pData->m_ProjectionMatrix = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(m_vFoV), m_AspectRatio, m_zNear, m_zFar);

    m_ProjectionDirty = false;
    m_InverseProjectionDirty = true;
}

void Camera::UpdateInverseProjectionMatrix() const
{
    if (m_ProjectionDirty)
    {
        UpdateProjectionMatrix();
    }

    pData->m_InverseProjectionMatrix = DirectX::XMMatrixInverse(nullptr, pData->m_ProjectionMatrix);
    m_InverseProjectionDirty = false;
}

const DirectX::XMVECTOR& Camera::GetPosition() const
{
    return pData->m_Translation;
}

const DirectX::XMVECTOR& Camera::GetDirection() const
{
    static DirectX::XMFLOAT4X4 tmpViewMatrix;
    static DirectX::XMVECTOR direction;
    static float x, y, z;

    if (m_ViewDirty)
    {
        UpdateViewMatrix();
    }

    XMStoreFloat4x4(
        &tmpViewMatrix,
        pData->m_ViewMatrix
    );

    x = tmpViewMatrix._13;
    y = tmpViewMatrix._23;
    z = tmpViewMatrix._33;

    direction = XMVectorSet(
        x,
        y,
        z,
        0.f
    );

    direction = XMVector3Normalize(direction);
    direction = DirectX::XMVectorSetW(direction, 1.0f);

    return direction;
}

