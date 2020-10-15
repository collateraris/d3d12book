struct Plane
{
    float3 N;   // Plane normal.
    float  d;   // Distance to origin.
};

struct Sphere
{
    float3 c;   // Center point.
    float  r;   // Radius.
};

struct Cone
{
    float3 T;   // Cone tip.
    float3 d;   // Direction of the cone.
    float  h;   // Height of the cone.
    float  r;   // bottom radius of the cone.
};

// Four planes of a view frustum (in view space).
// The planes are:
//  * Left,
//  * Right,
//  * Top,
//  * Bottom.
// The back and/or front planes can be computed from depth values in the 
// light culling compute shader.
struct Frustum
{
    Plane planes[4];   // left, right, top, bottom frustum planes.
};

struct ComputeShaderInput
{
    uint3 groupID           : SV_GroupID;           // 3D index of the thread group in the dispatch.
    uint3 groupThreadID     : SV_GroupThreadID;     // 3D index of local thread ID in a thread group.
    uint3 dispatchThreadID  : SV_DispatchThreadID;  // 3D index of global thread ID in the dispatch.
    uint  groupIndex        : SV_GroupIndex;        // Flattened local index of the thread within a thread group.
};

// Global variables
struct DispatchParams
{
    // Number of groups dispatched. (This parameter is not available as an HLSL system value!)
    uint3   numThreadGroups;
    uint3   numThreads;
};

struct ScreenToViewParams
{
    matrix InverseProjection;
    float2 ScreenDimensions;
};

void ClipToView(in float4 clip, in matrix inverseProjection, out float4 view);

// Convert screen space coordinates to view space.
void ScreenToView(in float4 screen, in float2 screenDim, in float4x4 inverseProjection, out float4 view);

void ComputePlane(in float3 p0, in float3 p1, in float3 p2, out Plane plane);

// Convert clip space coordinates to view space
void ClipToView(in float4 clip, in float4x4 inverseProjection, out float4 view)
{
    // View space position.
    view = mul(inverseProjection, clip);
    // Perspecitive projection.
    view = view / view.w;
};

// Convert screen space coordinates to view space.
void ScreenToView(in float4 screen, in float2 screenDim, in matrix inverseProjection, out float4 view)
{
    // Convert to normalized texture coordinates
    float2 texCoord = screen.xy / screenDim;

    // Convert to clip space
    float4 clip = float4(float2(texCoord.x, 1.0f - texCoord.y) * 2.0f - 1.0f, screen.z, screen.w);

    ClipToView(clip, inverseProjection, view);
};

// Compute a plane from 3 noncollinear points that form a triangle.
// This equation assumes a right-handed (counter-clockwise winding order) 
// coordinate system to determine the direction of the plane normal.
void ComputePlane(in float3 p0, in float3 p1, in float3 p2, out Plane plane)
{
    float3 v0 = p1 - p0;
    float3 v2 = p2 - p0;

    plane.N = normalize(cross(v0, v2));

    // Compute the distance to the origin using p0.
    plane.d = dot(plane.N, p0);
};

// it`s fake. not work
[numthreads(1, 1, 1)]
void main_fake(ComputeShaderInput IN)
{

}

