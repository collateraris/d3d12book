struct GRID_PARAMS
{
	matrix gridViewProjMatrices[3];
	float4 gridCellSizes;
	float4 gridPositions;
	float4 snappedGridPositions;
	float4 lastSnappedGridPositions;
	float4 globalIllumParams;
};

ConstantBuffer<GRID_PARAMS> bGridParams : register(b0);

struct VertexShaderOutput
{
	float4 Position    : POSITION;
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

static const float3 sc_viewDirections[3] =
{
	  float3(0.0f, 0.0f, -1.0f), // back to front
	  float3(-1.0f, 0.0f, 0.0f), // right to left
	  float3(0.0f, -1.0f, 0.0f)  // top to down 
};

uint GetViewIndex(in float3 normal)
{
	float3x3 directionMatrix;
	directionMatrix[0] = -sc_viewDirections[0];
	directionMatrix[1] = -sc_viewDirections[1];
	directionMatrix[2] = -sc_viewDirections[2];
	float3 dotProducts = abs(mul(directionMatrix, normal));
	float maximum = max(max(dotProducts.x, dotProducts.y), dotProducts.z);
	uint index;
	if (maximum == dotProducts.x)
		index = 0;
	else if (maximum == dotProducts.y)
		index = 1;
	else
		index = 2;
	return index;
};

[maxvertexcount(3)]
void main(triangle VertexShaderOutput input[3], inout TriangleStream<GeometryShaderOutput> outputStream)
{
	float3 faceNormal = normalize(input[0].Normal + input[1].Normal + input[2].Normal);

	// Get view, at which the current triangle is most visible, in order to achieve highest
	// possible rasterization of the primitive.
	uint viewIndex = GetViewIndex(faceNormal);

	GeometryShaderOutput output[3];
	[unroll]
	for (uint i = 0; i < 3; i++)
	{
		output[i].Position = mul(bGridParams.gridViewProjMatrices[viewIndex], input[i].Position);
		output[i].PositionWS = input[i].Position; // position in world space
		output[i].TexCoord = input[i].TexCoord;
		output[i].Normal = input[i].Normal;
	}

	// Bloat triangle in normalized device space with the texel size of the currently bound 
	// render-target. In this way pixels, which would have been discarded due to the low 
	// resolution of the currently bound render-target, will still be rasterized. 

	float2 side0N = normalize(output[0].Position.xy - output[1].Position.xy);
	float2 side1N = normalize(output[1].Position.xy - output[2].Position.xy);
	float2 side2N = normalize(output[2].Position.xy - output[0].Position.xy);
	const float texelSize = 1.0f / 64.0f;
	output[0].Position.xy += normalize(-side0N + side2N) * texelSize;
	output[1].Position.xy += normalize(side0N - side1N) * texelSize;
	output[2].Position.xy += normalize(side1N - side2N) * texelSize;

	[unroll]
	for (uint j = 0; j < 3; j++)
		outputStream.Append(output[j]);

	outputStream.RestartStrip();
}