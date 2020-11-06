#pragma once

#include <DirectXMath.h>

namespace dx12demo::core
{
	__declspec(align(16)) struct BSphere
	{
		DirectX::XMFLOAT3 pos;
		float r;
	};

	__declspec(align(16)) struct BAABB
	{
		DirectX::XMFLOAT3 box_min;
		DirectX::XMFLOAT3 box_max;
	};

    class CollectorBVData
    {
        const float INF_PLUS = 3.40282e+37;
        const float INF_MINUS = -INF_PLUS;
    public:
        CollectorBVData()
        {
            min_x = min_y = min_z = INF_PLUS;
            max_x = max_y = max_z = INF_MINUS;
        }

        void Collect(const DirectX::XMFLOAT3& pos)
        {
            float x = pos.x;
            float y = pos.y;
            float z = pos.z;

            if (max_x < x)
                max_x = x;
            if (max_y < y)
                max_y = y;
            if (max_z < z)
                max_z = z;
            if (min_x > x)
                min_x = x;
            if (min_y > y)
                min_y = y;
            if (min_z > z)
                min_z = z;
        };

        DirectX::XMFLOAT3 GetMin() const
        {
            return { min_x, min_y, min_z };
        }

        DirectX::XMFLOAT3 GetMax() const
        {
            return { max_x, max_y, max_z };
        }

        DirectX::XMFLOAT3 GetCenter() const
        {
            return { (max_x + min_x) * 0.5f, (max_y + min_y) * 0.5f, (max_z + min_z) * 0.5f };
        }

    private:
        float min_x, min_y, min_z;
        float max_x, max_y, max_z;
    };

}