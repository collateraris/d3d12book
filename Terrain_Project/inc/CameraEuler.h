#pragma once

#include <Events.h>
#include <DirectXMath.h>
#include <bitset>

namespace dx12demo
{
	class CameraEuler
	{
	public:

		CameraEuler() {};

		void XM_CALLCONV set_LookAt(const DirectX::FXMVECTOR& eye, const DirectX::FXMVECTOR& target, const DirectX::FXMVECTOR& up);

		void KeyProcessing(core::KeyEventArgs& e);

		void MouseProcessing(core::MouseMotionEventArgs& e, float mouseSpeed = 0.1f);

		void ScrollProcessing(core::MouseWheelEventArgs& e);

		void Movement(float deltaTime, float cameraSpeed = 5.f);

		DirectX::XMMATRIX get_ViewMatrix() const;
		DirectX::XMMATRIX get_InverseViewMatrix() const;

		void set_Projection(float fovy, float aspect, float zNear, float zFar);
		DirectX::XMMATRIX get_ProjectionMatrix() const;
		DirectX::XMMATRIX get_InverseProjectionMatrix() const;

		float get_FoV() const;

	private:

		CameraEuler(const CameraEuler& copy) = delete;
		CameraEuler operator=(const CameraEuler& copy) = delete;

		DirectX::XMVECTOR mCameraPos;
		DirectX::XMVECTOR mCameraFront;
		DirectX::XMVECTOR mCameraUp;

		// projection parameters
		float m_vFoV;   // Vertical field of view.
		float m_AspectRatio; // Aspect ratio
		float m_zNear;      // Near clip distance
		float m_zFar;       // Far clip distance.

		const int press_key_w = 0;
		const int press_key_s = 1;
		const int press_key_a = 2;
		const int press_key_d = 3;

		std::bitset<4> mKeyPresssedFlags;
	};
}