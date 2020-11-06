#define POINT_LIGHT 0
#define SPOT_LIGHT 1
#define DIRECTIONAL_LIGHT 2

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

struct Light
{
    /**
    * Position for point and spot lights (World space).
    */
    float4   PositionWS;
    //--------------------------------------------------------------( 16 bytes )
    /**
    * Direction for spot and directional lights (World space).
    */
    float4   DirectionWS;
    //--------------------------------------------------------------( 16 bytes )
    /**
    * Position for point and spot lights (View space).
    */
    float4   PositionVS;
    //--------------------------------------------------------------( 16 bytes )
    /**
    * Direction for spot and directional lights (View space).
    */
    float4   DirectionVS;
    //--------------------------------------------------------------( 16 bytes )
    /**
    * Color of the light. Diffuse and specular colors are not seperated.
    */
    float4   Color;
    //--------------------------------------------------------------( 16 bytes )
    /**
    * The half angle of the spotlight cone.
    */
    float    SpotAngle;
    /**
    * The range of the light.
    */
    float    Range;

    /**
     * The intensity of the light.
     */
    float    Intensity;

    /**
     * The attenuation of the light.
     */
    float    Attenuation;

    /**
    * Disable or enable the light.
    */
    bool    Enabled;
    //--------------------------------------------------------------( 16 bytes )

    /**
     * Is the light selected in the editor?
     */
    bool    Selected;

    /**
    * The type of the light.
    */
    uint    Type;
    float2  Padding;
    //--------------------------------------------------------------( 16 bytes )
    //--------------------------------------------------------------( 16 * 7 = 112 bytes )
};

struct LightResult
{
    float4 Diffuse;
    float4 Specular;
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

bool SphereInsidePlane(in Sphere sphere, in Plane plane);

bool SphereInsideFrustum(in Sphere sphere, in Frustum frustum, float zNear, float zFar);

bool ConeInsidePlane(in Cone cone, in Plane plane);

bool PointInsidePlane(in float3 p, in Plane plane);

bool ConeInsideFrustum(in Cone cone, in Frustum frustum, float zNear, float zFar);

float3 ExpandNormal(in float3 n);

float3 DoNormalMapping(in float3 normalFromTex, in float3x3 TBN);

float DoAttenuation(float attenuation, float distance);

float DoAttenuationByRange(float range, float distance);

float DoDiffuse(in float3 N, in float3 L);

float DoSpecular(in float3 V, in float3 N, in float3 L);

float DoSpotCone(in float3 spotDir, in float3 L, in float spotAngle);

void DoPointLight(in Light light, in float3 V, in float3 P, in float3 N, out LightResult result);

void DoSpotLight(in Light light, in float3 V, in float3 P, in float3 N, out LightResult result);

void DoDirectionalLight(in Light light, in float3 V, float3 P, float3 N, out LightResult result);

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

bool SphereInsidePlane(in Sphere sphere, in Plane plane)
{
    return dot(sphere.c, plane.N) - plane.d < -sphere.r;
};

bool SphereInsideFrustum(in Sphere sphere, in Frustum frustum, float zNear, float zFar)
{
    // First check depth
    // Note: Here, the view vector points in the -Z axis so the 
    // far depth value will be approaching -infinity.
    if (sphere.c.z - sphere.r > zNear || sphere.c.z + sphere.r < zFar)
    {
        return false;
    }

    // Then check frustum planes
    for (int i = 0; i < 4; i++)
    {
        if (SphereInsidePlane(sphere, frustum.planes[i]))
        {
            return false;
        }
    }

    return true;
};

bool ConeInsidePlane(in Cone cone, in Plane plane)
{
    // Compute the farthest point on the end of the cone to the positive space of the plane.
    float3 m = cross(cross(plane.N, cone.d), cone.d);
    float3 Q = cone.T + cone.d * cone.h - cone.r * m;

    // The cone is in the negative halfspace of the plane if both
    // the tip of the cone and the farthest point on the end of the cone to the 
    // positive halfspace of the plane are both inside the negative halfspace 
    // of the plane.
    return PointInsidePlane(cone.T, plane) && PointInsidePlane(Q, plane);
};

bool ConeInsideFrustum(in Cone cone, in Frustum frustum, float zNear, float zFar)
{
    Plane nearPlane = { float3(0, 0, -1), -zNear };
    Plane farPlane = { float3(0, 0, 1), zFar };

    // First check the near and far clipping planes.
    if (ConeInsidePlane(cone, nearPlane) || ConeInsidePlane(cone, farPlane))
    {
        return false;
    }

    // Then check frustum planes
    for (int i = 0; i < 4; i++)
    {
        if (ConeInsidePlane(cone, frustum.planes[i]))
        {
            return false;
        }
    }

    return true;
}

bool PointInsidePlane(in float3 p, in Plane plane)
{
    return dot(plane.N, p) - plane.d < 0;
};

float3 ExpandNormal(in float3 n)
{
    return n * 2.0f - 1.0f;
}

float3 DoNormalMapping(in float3 normalFromTex, in float3x3 TBN)
{
    float3 normal = ExpandNormal(normalFromTex);
    normal = mul(normal, TBN);
    return normalize(normal);
}

float DoAttenuation(float attenuation, float distance)
{
    return 1.0f / (1.0f + attenuation * distance * distance);
}

float DoAttenuationByRange(float range, float distance)
{
    return 1.0f - smoothstep(range * 0.75f, range, distance);
}

float DoDiffuse(in float3 N, in float3 L)
{
    return max(0, dot(N, L));
}

float DoSpecular(in float3 V, in float3 N, in float3 L)
{
    float3 R = normalize(reflect(-L, N));
    float RdotV = max(0, dot(R, V));

    const float specularPower = 16;
    return pow(RdotV, specularPower);
}

float DoSpotCone(in float3 spotDir, in float3 L, in float spotAngle)
{
    float minCos = cos(spotAngle);
    float maxCos = (minCos + 1.0f) / 2.0f;

    float cosAngle = dot(spotDir, -L);
    return smoothstep(minCos, maxCos, cosAngle);
}

void DoPointLight(in Light light, in float3 V, in float3 P, in float3 N, out LightResult result)
{
    float3 L = (light.PositionVS.xyz - P);
    float d = length(L);
    L = L / d;

    float attenuation = DoAttenuationByRange(light.Range, d);

    result.Diffuse = light.Color * DoDiffuse(N, L) * attenuation  * light.Intensity;
    result.Specular = light.Color * DoSpecular(V, N, L) * attenuation *  light.Intensity;
}

void DoSpotLight(in Light light, in float3 V, in float3 P, in float3 N, out LightResult result)
{
    float3 L = (light.PositionVS.xyz - P);
    float d = length(L);
    L = L / d;

    float attenuation = DoAttenuationByRange(light.Range, d);
    float spotIntensity = DoSpotCone(light.DirectionVS.xyz, L, light.SpotAngle);

    result.Diffuse = light.Color * DoDiffuse(N, L) * attenuation * spotIntensity *  light.Intensity;
    result.Specular = light.Color * DoSpecular(V, N, L) * attenuation * spotIntensity * light.Intensity;
}

void DoDirectionalLight(in Light light, in float3 V, float3 P, float3 N, out LightResult result)
{
    float4 L = normalize(-light.DirectionVS);

    result.Diffuse = light.Color * DoDiffuse(N, L) * light.Intensity;
    result.Specular = light.Color * DoSpecular(V, N, L) * light.Intensity;
}

// it`s fake. not work
[numthreads(1, 1, 1)]
void main_fake(ComputeShaderInput IN)
{

}

