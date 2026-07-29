#include "Shared/GpuStructs.h"
#include "Shared/Bindings.h"

import Common.Sphere;
import Common.GI;
import Common.Constants;

[[vk::binding(BIND_GLOBAL_CAMERA, 0)]] StructuredBuffer<CameraData>     camera;
[[vk::binding(BIND_GLOBAL_POINT_LIGHTS, 0)]] StructuredBuffer<PointLightData> pointLights;

[[vk::binding(BIND_GLOBAL_SH_GRID_INFO, 0)]] StructuredBuffer<SHGridInfo> shGridInfo;

struct PushConstants
{
    float3 probePos;
    float  scale;
    uint   faceIdx;
};
[[vk::push_constant]] ConstantBuffer<PushConstants> pc;

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
    o.pos      = bakeFaceClip(worldPos, pc.probePos, pc.faceIdx, shGridInfo[0].captureRange);
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
