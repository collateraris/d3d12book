#pragma once

#include <URootObject.h>

#include <DirectXMath.h>

#include <array>
#include <vector>

namespace dx12demo::core
{
	enum EFrustumPlane
	{
		_NEAR = 0,
		_FAR = 1,
		_LEFT = 2,
		_RIGHT = 3,
		_TOP = 4,
		_BOTTOM = 5,
	};

	struct BSphere;
	struct BAABB;
	class Frustum : public URootObject
	{
	public:
		Frustum();
		virtual ~Frustum();

		void ConstructFrustum(float farPlanePar, const DirectX::XMMATRIX& projectionMatrix, const DirectX::XMMATRIX& viewMatrix);

		const std::array<DirectX::XMVECTOR, 6>& GetFrustumPlanesVector() const;

		const std::array<DirectX::XMFLOAT4, 6>& GetFrustumPlanesF4() const;

		static void CullingAABB(const std::vector<BAABB>& aabb_data, std::vector<int>& culling_res, const std::array<DirectX::XMFLOAT4, 6> frustum_planes);

		static void CullingSpheres(const std::vector<BSphere>& sphere_data, std::vector<int>& culling_res, const std::array<DirectX::XMFLOAT4, 6> frustum_planes);

		static bool FrustumInSphere(const BSphere& sphere_data, const std::array<DirectX::XMFLOAT4, 6>& planes);

		static bool FrustumInAABB(const BAABB& aabb_data, const std::array<DirectX::XMFLOAT4, 6>& planes);

		static void SSECullingSpheres(std::vector<BSphere>& sphere_data, std::vector<int>& culling_res, const std::array<DirectX::XMVECTOR, 6>& planes);

	private:

		std::array<DirectX::XMVECTOR, 6> m_planes_vector;
		std::array<DirectX::XMFLOAT4, 6> m_planes_f4;
	};
}