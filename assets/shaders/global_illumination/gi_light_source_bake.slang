struct CameraData
{
    float4x4 cameraSpaceMatrix;
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float4x4 invViewProj;
    float4   cameraPositionAndPadding;
    float4   frustumPlanes[6];
};

struct PointLightData
{
    float3 position;      float radius;
    float3 color;         float intensity;
    float3 direction;     float innerConeAngle;
    float  outerConeAngle;
    uint   type;
    uint   _p1;
    uint   _p2;
};

[[vk::binding(0, 0)]] StructuredBuffer<CameraData>     camera;
[[vk::binding(2, 0)]] StructuredBuffer<PointLightData> pointLights;

struct PushConstants { float scale; };
[[vk::push_constant]] ConstantBuffer<PushConstants> pc;

// UV sphere: N_LAT rings x N_LON segments, 6 verts per quad (same as gi_probe_debug)
static const int   N_LAT = 8;
static const int   N_LON = 8;
static const float PI    = 3.14159265f;

float3 spherePoint(float phi, float theta)
{
    return float3(sin(phi) * cos(theta), cos(phi), sin(phi) * sin(theta));
}

struct VSOut
{
    float4 pos      : SV_Position;
    float3 color    : TEXCOORD0;
    float  intensity : TEXCOORD1;
};

[shader("vertex")]
VSOut vertMain(uint vid : SV_VertexID, uint iid : SV_InstanceID)
{
    int quadIdx    = int(vid) / 6;
    int vertInQuad = int(vid) % 6;
    int lat = quadIdx / N_LON;
    int lon = quadIdx % N_LON;

    int dlat, dlon;
    if      (vertInQuad == 0) { dlat = 0; dlon = 0; }
    else if (vertInQuad == 1) { dlat = 1; dlon = 1; }
    else if (vertInQuad == 2) { dlat = 1; dlon = 0; }
    else if (vertInQuad == 3) { dlat = 0; dlon = 0; }
    else if (vertInQuad == 4) { dlat = 0; dlon = 1; }
    else                      { dlat = 1; dlon = 1; }

    float phi   = PI * float(lat + dlat) / float(N_LAT);
    float theta = 2.0f * PI * float(lon + dlon) / float(N_LON);

    float3 localNormal = spherePoint(phi, theta);
    float3 worldPos    = pointLights[iid].position + localNormal * pc.scale;

    VSOut o;
    o.pos      = mul(camera[0].cameraSpaceMatrix, float4(worldPos, 1.0f));
    o.color    = pointLights[iid].color;
    o.intensity = pointLights[iid].intensity;
    return o;
}

[shader("fragment")]
float4 fragMain(float3 color : TEXCOORD0, float intensity : TEXCOORD1) : SV_Target
{
    // Output raw HDR radiance — no tonemapping so SH projection captures true emission
    return float4(color * intensity, 1.0f);
}
