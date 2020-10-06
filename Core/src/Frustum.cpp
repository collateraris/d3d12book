#include <Frustum.h>

#include <BoundingVolumesPrimitive.h>

#include <xmmintrin.h>
#include <mmintrin.h>
#include <emmintrin.h>

using namespace dx12demo::core;

Frustum::Frustum()
{

}

Frustum::~Frustum()
{

}

void Frustum::ConstructFrustum(float farPlanePar, const DirectX::XMMATRIX& projectionMatrix, const DirectX::XMMATRIX& viewMatrix)
{
	float zMinimum, r;

	DirectX::XMFLOAT4X4 fProj;
	DirectX::XMStoreFloat4x4(&fProj, projectionMatrix);

	// Calculate the minimum Z distance in the frustum.
	zMinimum = -fProj._43 / fProj._33;
	r = farPlanePar / (farPlanePar - zMinimum);
	fProj._33 = r;
	fProj._43 = -r * zMinimum;
	DirectX::XMMATRIX projMatrix = DirectX::XMLoadFloat4x4(&fProj);

	// Create the frustum matrix from the view matrix and updated projection matrix.
	DirectX::XMMATRIX matrix = DirectX::XMMatrixMultiply(viewMatrix, projMatrix);

	DirectX::XMFLOAT4X4 fMatrix;
	DirectX::XMStoreFloat4x4(&fMatrix, matrix);

	// Calculate near plane of frustum.
	DirectX::XMFLOAT4 nearPlane;
	nearPlane.x = fMatrix._14 + fMatrix._13;
	nearPlane.y = fMatrix._24 + fMatrix._23;
	nearPlane.z = fMatrix._34 + fMatrix._33;
	nearPlane.w = fMatrix._44 + fMatrix._43;
	m_planes_vector[EFrustumPlane::_NEAR] = DirectX::XMLoadFloat4(&nearPlane);
	m_planes_vector[EFrustumPlane::_NEAR] = DirectX::XMVector4Normalize(m_planes_vector[EFrustumPlane::_NEAR]);

	// Calculate far plane of frustum.
	DirectX::XMFLOAT4 farPlane;
	farPlane.x = fMatrix._14 - fMatrix._13;
	farPlane.y = fMatrix._24 - fMatrix._23;
	farPlane.z = fMatrix._34 - fMatrix._33;
	farPlane.w = fMatrix._44 - fMatrix._43;
	m_planes_vector[EFrustumPlane::_FAR] = DirectX::XMLoadFloat4(&farPlane);
	m_planes_vector[EFrustumPlane::_FAR] = DirectX::XMVector4Normalize(m_planes_vector[EFrustumPlane::_FAR]);

	// Calculate left plane of frustum.
	DirectX::XMFLOAT4 leftPlane;
	leftPlane.x = fMatrix._14 + fMatrix._11;
	leftPlane.y = fMatrix._24 + fMatrix._21;
	leftPlane.z = fMatrix._34 + fMatrix._31;
	leftPlane.w = fMatrix._44 + fMatrix._41;
	m_planes_vector[EFrustumPlane::_LEFT] = DirectX::XMLoadFloat4(&leftPlane);
	m_planes_vector[EFrustumPlane::_LEFT] = DirectX::XMVector4Normalize(m_planes_vector[EFrustumPlane::_LEFT]);

	// Calculate right plane of frustum.
	DirectX::XMFLOAT4 rightPlane;
	rightPlane.x = fMatrix._14 - fMatrix._11;
	rightPlane.y = fMatrix._24 - fMatrix._21;
	rightPlane.z = fMatrix._34 - fMatrix._31;
	rightPlane.w = fMatrix._44 - fMatrix._41;
	m_planes_vector[EFrustumPlane::_RIGHT] = DirectX::XMLoadFloat4(&rightPlane);
	m_planes_vector[EFrustumPlane::_RIGHT] = DirectX::XMVector4Normalize(m_planes_vector[EFrustumPlane::_RIGHT]);

	// Calculate top plane of frustum.
	DirectX::XMFLOAT4 topPlane;
	topPlane.x = fMatrix._14 - fMatrix._12;
	topPlane.y = fMatrix._24 - fMatrix._22;
	topPlane.z = fMatrix._34 - fMatrix._32;
	topPlane.w = fMatrix._44 - fMatrix._42;
	m_planes_vector[EFrustumPlane::_TOP] = DirectX::XMLoadFloat4(&topPlane);
	m_planes_vector[EFrustumPlane::_TOP] = DirectX::XMVector4Normalize(m_planes_vector[EFrustumPlane::_TOP]);

	// Calculate bottom plane of frustum.
	DirectX::XMFLOAT4 bottomPlane;
	bottomPlane.x = fMatrix._14 + fMatrix._12;
	bottomPlane.y = fMatrix._24 + fMatrix._22;
	bottomPlane.z = fMatrix._34 + fMatrix._32;
	bottomPlane.w = fMatrix._44 + fMatrix._42;
	m_planes_vector[EFrustumPlane::_BOTTOM] = DirectX::XMLoadFloat4(&bottomPlane);
	m_planes_vector[EFrustumPlane::_BOTTOM] = DirectX::XMVector4Normalize(m_planes_vector[EFrustumPlane::_BOTTOM]);

	for (int j = 0; j < 6; ++j)
	{
		DirectX::XMStoreFloat4(&m_planes_f4[j], m_planes_vector[j]);
	}
}

const std::array<DirectX::XMVECTOR, 6>& Frustum::GetFrustumPlanesVector() const
{
	return m_planes_vector;
}

const std::array<DirectX::XMFLOAT4, 6>& Frustum::GetFrustumPlanesF4() const
{
	return m_planes_f4;
}

void Frustum::SSECullingSpheres(std::vector<BSphere>& sphere_data, std::vector<int>& culling_res, const std::array<DirectX::XMVECTOR, 6>& planes)
{
	float* sphere_data_ptr = reinterpret_cast<float*>(sphere_data.data());
	int* culling_res_sse = culling_res.data();

	//xyzw components is collected in separete vectors for optimization
	__m128 zero_v = _mm_setzero_ps();
	__m128 frustum_planes_x[6];
	__m128 frustum_planes_y[6];
	__m128 frustum_planes_z[6];
	__m128 frustum_planes_d[6];

	const int FRUSTUM_PLANES_NUM = 6;
	for (int i = 0; i < FRUSTUM_PLANES_NUM; i++)
	{
		DirectX::XMFLOAT4 frustum_plane;
		DirectX::XMStoreFloat4(&frustum_plane, planes[i]);

		frustum_planes_x[i] = _mm_set1_ps(frustum_plane.x);
		frustum_planes_y[i] = _mm_set1_ps(frustum_plane.y);
		frustum_planes_z[i] = _mm_set1_ps(frustum_plane.z);
		frustum_planes_d[i] = _mm_set1_ps(frustum_plane.w);
	}

	//we process 4 objects per step
	const int CULL_OBJECTS_PER_ITERATION = 4;
	int num_objects = sphere_data.size();
	for (int i = 0; i < num_objects; i += CULL_OBJECTS_PER_ITERATION)
	{
		__m128 spheres_pos_x = _mm_load_ps(sphere_data_ptr);
		__m128 spheres_pos_y = _mm_load_ps(sphere_data_ptr + 4);
		__m128 spheres_pos_z = _mm_load_ps(sphere_data_ptr + 8);
		__m128 spheres_radius = _mm_load_ps(sphere_data_ptr + 12);
		sphere_data_ptr += 16;

		//but for our calculations we need transpose data, to collect x, y, z and w coordinates in separate vectors
		_MM_TRANSPOSE4_PS(spheres_pos_x, spheres_pos_y, spheres_pos_z, spheres_radius);

		__m128 spheres_neg_radius = _mm_sub_ps(zero_v, spheres_radius);

		__m128 intersection_res = _mm_setzero_ps();

		for (int j = 0; j < FRUSTUM_PLANES_NUM; j++)
		{
			//1. calc distance to plane dot(sphere_pos.xyz, plane.xyz) + plane.w
			//2. if distance < sphere radius, then sphere outside frustum

			__m128 dot_x = _mm_mul_ps(spheres_pos_x, frustum_planes_x[j]);
			__m128 dot_y = _mm_mul_ps(spheres_pos_y, frustum_planes_y[j]);
			__m128 dot_z = _mm_mul_ps(spheres_pos_z, frustum_planes_z[j]);

			__m128 sum_xy = _mm_add_ps(dot_x, dot_y);
			__m128 sum_zw = _mm_add_ps(dot_z, frustum_planes_d[j]);

			__m128 distance_to_plane = _mm_add_ps(sum_xy, sum_zw);

			__m128 plane_res = _mm_cmple_ps(distance_to_plane, spheres_neg_radius);

			intersection_res = _mm_or_ps(intersection_res, plane_res);
		}

		__m128i intersection_res_i = _mm_cvtps_epi32(intersection_res);
		_mm_store_si128((__m128i*)&culling_res_sse[i], intersection_res_i);
	}
}

void Frustum::CullingSpheres(const std::vector<BSphere>& sphere_data, std::vector<int>& culling_res, const std::array<DirectX::XMFLOAT4, 6> frustum_planes)
{
	for (int i = 0; i < sphere_data.size(); ++i)
	{
		bool inside = Frustum::FrustumInSphere(sphere_data[i], frustum_planes);
		culling_res[i] = inside ? 0 : -1;
	}
}

void Frustum::CullingAABB(const std::vector<BAABB>& aabb_data, std::vector<int>& culling_res, std::array<DirectX::XMFLOAT4, 6> frustum_planes)
{
	for (int i = 0; i < aabb_data.size(); ++i)
	{
		bool inside = Frustum::FrustumInAABB(aabb_data[i], frustum_planes);
		culling_res[i] = inside ? 0 : -1;
	}
}

bool Frustum::FrustumInSphere(const BSphere& sphere_data, const std::array<DirectX::XMFLOAT4, 6>& planes)
{
	auto& pos = sphere_data.pos;
	auto radius = sphere_data.r;
	bool inside = true;
	for (int j = 0; j < 6; ++j)
	{
		auto& frustum_plane = planes[j];
		if (frustum_plane.x * pos.x + frustum_plane.y * pos.y + frustum_plane.z * pos.z + frustum_plane.w <= -radius)
		{
			inside = false;
		}
	}
	return inside;
}

bool Frustum::FrustumInAABB(const BAABB& aabb_data, const std::array<DirectX::XMFLOAT4, 6>& planes)
{
	auto& Max = aabb_data.box_max;
	auto& Min = aabb_data.box_min;
	bool inside = true;
	for (int j = 0; j < 6; ++j)
	{
		float d = std::max(Min.x * planes[j].x, Max.x * planes[j].x)
			+ std::max(Min.y * planes[j].y, Max.y * planes[j].y)
			+ std::max(Min.z * planes[j].z, Max.z * planes[j].z)
			+ planes[j].w;
		inside &= d > 0;
	}
	return inside;
}