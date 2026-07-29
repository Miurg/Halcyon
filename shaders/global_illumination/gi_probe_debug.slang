#include "Shared/GpuStructs.h"
#include "Shared/Bindings.h"

import Common.Sphere;
import Common.Constants;
import Common.SH;

[[vk::binding(BIND_GLOBAL_CAMERA, 0)]] StructuredBuffer<CameraData>   camera;
[[vk::binding(BIND_GLOBAL_SH_PROBES, 0)]] StructuredBuffer<SHProbeEntry> shProbes;

// Push constants carry live grid parameters so positions update without rebaking.
// SH coloring still reads from the baked probe buffer (slot = iid + 1).
struct PushConstants
{
    float3 origin;   // grid world-space origin
    float  scale;    // sphere radius
    int    countX;   // probes per axis X
    int    countY;   // probes per axis Y
    int    countZ;   // probes per axis Z
    float  spacing;  // meters between probes
};
[[vk::push_constant]] ConstantBuffer<PushConstants> pc;

struct VSOut
{
    float4 pos    : SV_Position;
    float3 normal : TEXCOORD0;
    nointerpolation uint probeIdx : TEXCOORD1;
};

[shader("vertex")]
VSOut vertMain(uint vid : SV_VertexID, uint iid : SV_InstanceID)
{
    // Decode 3D grid index from flat instance ID (matches baking loop order iz/iy/ix)
    int ix = int(iid) % pc.countX;
    int iy = (int(iid) / pc.countX) % pc.countY;
    int iz = int(iid) / (pc.countX * pc.countY);
    float3 probePos = pc.origin + pc.spacing * float3(float(ix), float(iy), float(iz));

    // SH coloring reads from the baked buffer; slot 0 is skybox, scene probes start at 1
    uint probeSlot = iid + 1u;

    // Decode which quad and vertex within the quad
    int quadIdx    = int(vid) / 6;
    int vertInQuad = int(vid) % 6;
    int lat = quadIdx / N_LON;
    int lon = quadIdx % N_LON;

    // Quad corners: two triangles, CCW winding when viewed from OUTSIDE the sphere.
    // Verified with cross product: outward normal = positive radial direction.
    // Tri0: (lat,lon), (lat+1,lon+1), (lat+1,lon)
    // Tri1: (lat,lon), (lat,lon+1),   (lat+1,lon+1)
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
    float3 worldPos    = probePos + localNormal * pc.scale;

    VSOut o;
    o.pos      = mul(camera[0].cameraSpaceMatrix, float4(worldPos, 1.0f));
    o.normal   = localNormal;
    o.probeIdx = probeSlot;
    return o;
}

[shader("fragment")]
float4 fragMain(float3 normal : TEXCOORD0, nointerpolation uint probeIdx : TEXCOORD1) : SV_Target
{
    float3 n = normalize(normal);

    SHProbeEntry p = shProbes[probeIdx];
    float3 sh[4] = {p.sh0, p.sh1, p.sh2, p.sh3};

    float3 irr = computeIrradiance(n, sh);
    // Reinhard with white point = PI
    irr = irr / (irr + float3(PI, PI, PI));
    return float4(irr, 1.0f);
}
