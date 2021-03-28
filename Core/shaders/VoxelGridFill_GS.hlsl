struct GRID_PARAMS
{
	matrix gridProjMatrices;
	float4 gridCellSizes;
	float4 gridPositions;
	float4 snappedGridPositions;
	float4 lastSnappedGridPositions;
	float4 globalIllumParams;
};

ConstantBuffer<GRID_PARAMS> bGridParams : register(b1);

struct VertexShaderOutput
{
	float4 Position    : SV_POSITION;
	float3 PositionWS  : POS_WS;
	float3 Normal      : NORMAL;
	float2 TexCoord    : TEXCOORD;
};

struct GeometryShaderOutput
{
	float4 Position    : SV_POSITION;
	float3 PositionWS  : POS_WS;
	float3 Normal      : NORMAL;
	float2 TexCoord    : TEXCOORD;
};

[maxvertexcount(3)]
void main(triangle VertexShaderOutput input[3], inout TriangleStream<GeometryShaderOutput> outputStream)
{

	float4x4 rotMatX = { 1.0f, 0.0f, 0.0f, 0.0f,
					0.0f, 0.0f, -1.0f, 0.0f,
					0.0f, 1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 0.0f, 1.0f };

	float4x4 rotMatY = { 0.0f, 0.0f, 1.0f, 0.0f,
						0.0f, 1.0f, 0.0f, 0.0f,
						-1.0f, 0.0f, 0.0f, 0.0f,
						0.0f, 0.0f, 0.0f, 1.0f };

	float4x4 rotMat = { 1.0f, 0.0f, 0.0f, 0.0f,
						0.0f, 1.0f, 0.0f, 0.0f,
						0.0f, 0.0f, 1.0f, 0.0f,
						0.0f, 0.0f, 0.0f, 1.0f };

	float minScale = 1.0f;
	float minLen = bGridParams.gridCellSizes.x * 0.666f;
	if (length(input[0].PositionWS - input[1].PositionWS) < minLen)
		minScale = max(minScale, minLen / length(input[0].PositionWS - input[1].PositionWS));

	if (length(input[1].PositionWS - input[2].PositionWS) < minLen)
		minScale = max(minScale, minLen / length(input[0].PositionWS - input[2].PositionWS));

	if (length(input[2].PositionWS - input[0].PositionWS) < minLen)
		minScale = max(minScale, minLen / length(input[2].PositionWS - input[0].PositionWS));

	for (int i = 0; i < 3; i++)
		input[i].PositionWS *= minScale;


	// calculate normal as orientation of triangle
	float3 faceNormal = normalize(cross(input[0].PositionWS.xyz - input[1].PositionWS.xyz, input[0].PositionWS.xyz - input[2].PositionWS.xyz));

	// select the most valuable axis
	if (faceNormal.x > faceNormal.y && faceNormal.x > faceNormal.z)
	{
		rotMat = rotMatY;
	}
	else if (faceNormal.y > faceNormal.z)
	{
		rotMat = rotMatX;
	}

	GeometryShaderOutput output[3];
	[unroll]
	for (uint i = 0; i < 3; i++)
	{
		output[i].Position = input[i].Position;
			//mul(bGridParams.gridProjMatrices, mul(rotMat, input[i].PositionWS));
		output[i].PositionWS = input[i].PositionWS; // position in world space
		output[i].TexCoord = input[i].TexCoord;
		output[i].Normal = input[i].Normal;
	}

	
	[unroll]
	for (uint j = 0; j < 3; j++)
		outputStream.Append(output[j]);

	outputStream.RestartStrip();
}