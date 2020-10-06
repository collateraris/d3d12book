#include <CameraEuler.h>

#include <algorithm>

using namespace dx12demo;
using namespace DirectX;

void XM_CALLCONV CameraEuler::set_LookAt(const DirectX::FXMVECTOR& eye, const DirectX::FXMVECTOR& target, const DirectX::FXMVECTOR& up)
{
	mCameraPos = eye;
	mCameraFront = DirectX::XMVectorSubtract(target, mCameraPos);
	mCameraUp = up;
}

DirectX::XMMATRIX CameraEuler::get_ViewMatrix() const
{
	return DirectX::XMMatrixLookAtLH(mCameraPos, DirectX::XMVectorAdd(mCameraPos, mCameraFront), mCameraUp);
}

DirectX::XMMATRIX CameraEuler::get_InverseViewMatrix() const
{
	return DirectX::XMMatrixInverse(nullptr, 
		DirectX::XMMatrixLookAtLH(mCameraPos, DirectX::XMVectorAdd(mCameraPos, mCameraFront), mCameraUp));
}

void CameraEuler::set_Projection(float fovy, float aspect, float zNear, float zFar)
{
	m_vFoV = fovy;
	m_AspectRatio = aspect;
	m_zNear = zNear;
	m_zFar = zFar;
}

DirectX::XMMATRIX CameraEuler::get_ProjectionMatrix() const
{
	return DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(m_vFoV), m_AspectRatio, m_zNear, m_zFar);
}

DirectX::XMMATRIX CameraEuler::get_InverseProjectionMatrix() const
{
	return DirectX::XMMatrixInverse(nullptr, 
		DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(m_vFoV), m_AspectRatio, m_zNear, m_zFar));
}

float CameraEuler::get_FoV() const
{
	return m_vFoV;
}

void CameraEuler::ScrollProcessing(core::MouseWheelEventArgs& e)
{
	m_vFoV -= e.WheelDelta;
	m_vFoV = std::clamp(m_vFoV, 12.0f, 90.0f);
}

void CameraEuler::MouseProcessing(core::MouseMotionEventArgs& e, float mouseSpeed/* = 0.1f*/)
{
	static float yaw = -90.0f;
	static float pitch = 0.0f;

	pitch -= e.RelY * mouseSpeed;
	pitch = std::clamp(pitch, -89.0f, 89.0f);
	yaw += e.RelX * mouseSpeed;

	float front_x = cos(DirectX::XMConvertToRadians(pitch)) * cos(DirectX::XMConvertToRadians(yaw));
	float front_y = sin(DirectX::XMConvertToRadians(pitch));
	float front_z = cos(DirectX::XMConvertToRadians(pitch)) * sin(DirectX::XMConvertToRadians(yaw));

	mCameraFront = DirectX::XMVector3Normalize({front_x, front_y, front_z});
}

void CameraEuler::KeyProcessing(core::KeyEventArgs& e)
{
	if (e.Key == KeyCode::A)
	{
		if (e.State == e.Pressed)
		{
			mKeyPresssedFlags.set(press_key_a);
		}
		else if (e.State == e.Released)
		{
			mKeyPresssedFlags.reset(press_key_a);
		}
	}
	else if (e.Key == KeyCode::W)
	{
		if (e.State == e.Pressed)
		{
			mKeyPresssedFlags.set(press_key_w);
		}
		else if (e.State == e.Released)
		{
			mKeyPresssedFlags.reset(press_key_w);
		}
	}
	else if (e.Key == KeyCode::D)
	{
		if (e.State == e.Pressed)
		{
			mKeyPresssedFlags.set(press_key_d);
		}
		else if (e.State == e.Released)
		{
			mKeyPresssedFlags.reset(press_key_d);
		}
	}
	else if (e.Key == KeyCode::S)
	{
		if (e.State == e.Pressed)
		{
			mKeyPresssedFlags.set(press_key_s);
		}
		else if (e.State == e.Released)
		{
			mKeyPresssedFlags.reset(press_key_s);
		}
	}
}

void CameraEuler::Movement(float deltaTime, float cameraSpeed/* = 5.f*/)
{
	cameraSpeed *= deltaTime;

	if (mKeyPresssedFlags.test(press_key_a))
	{
		auto cross_res = DirectX::XMVector3Cross(mCameraFront, mCameraUp);
		auto normalized = DirectX::XMVector3Normalize(cross_res) * cameraSpeed;
		mCameraPos = DirectX::XMVectorSubtract(mCameraPos, normalized);
	}
	if (mKeyPresssedFlags.test(press_key_w))
	{
		mCameraPos = DirectX::XMVectorAdd(mCameraPos, mCameraFront * cameraSpeed);
	}
	if (mKeyPresssedFlags.test(press_key_d))
	{
		auto cross_res = DirectX::XMVector3Cross(mCameraFront, mCameraUp);
		auto normalized = DirectX::XMVector3Normalize(cross_res) * cameraSpeed;
		mCameraPos = DirectX::XMVectorAdd(mCameraPos, normalized);
	}
	if (mKeyPresssedFlags.test(press_key_s))
	{
		mCameraPos = DirectX::XMVectorSubtract(mCameraPos, mCameraFront * cameraSpeed);
	}
}