struct CameraData
{
    float4x4 cameraSpaceMatrix;
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float4x4 invViewProj;
    float4   cameraPositionAndPadding;
    float4   frustumPlanes[6];
};

struct SHProbeEntry
{
    float3 position;      float influenceRadius;
    float3 sh0; float _p0;
    float3 sh1; float _p1;
    float3 sh2; float _p2;
    float3 sh3; float _p3;
    float3 sh4; float _p4;
    float3 sh5; float _p5;
    float3 sh6; float _p6;
    float3 sh7; float _p7;
    float3 sh8; float _p8;
};

[[vk::binding(0, 0)]] StructuredBuffer<CameraData>   camera;
[[vk::binding(4, 0)]] StructuredBuffer<SHProbeEntry> shProbes;

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

// UV sphere: N_LAT rings × N_LON segments, 2 triangles per quad = 6 verts per quad
static const int N_LAT = 8;
static const int N_LON = 8;

static const float PI = 3.14159265f;

float3 spherePoint(float phi, float theta)
{
    return float3(sin(phi) * cos(theta), cos(phi), sin(phi) * sin(theta));
}

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

// Evaluates L2 SH irradiance in direction n.
// Coefficients match computeIrradiance() in global_illumination_forward.slang.
float3 computeIrradiance(float3 n, SHProbeEntry p)
{
    const float f0 = 0.28209479f;
    const float f1 = 0.48860251f;
    const float f2 = 1.09254843f;
    const float f3 = 0.31539156f;
    const float f4 = 0.54627421f;

    float3 c = p.sh0 * f0;
    c += p.sh1 * (-f1 * n.y);
    c += p.sh2 * ( f1 * n.z);
    c += p.sh3 * (-f1 * n.x);
    c += p.sh4 * ( f2 * n.x * n.y);
    c += p.sh5 * (-f2 * n.y * n.z);
    c += p.sh6 * ( f3 * (3.0f * n.z * n.z - 1.0f));
    c += p.sh7 * (-f2 * n.x * n.z);
    c += p.sh8 * ( f4 * (n.x * n.x - n.y * n.y));
    return max(c, float3(0.0f, 0.0f, 0.0f));
}

[shader("fragment")]
float4 fragMain(float3 normal : TEXCOORD0, nointerpolation uint probeIdx : TEXCOORD1) : SV_Target
{
    float3 n   = normalize(normal);
    float3 irr = computeIrradiance(n, shProbes[probeIdx]);
    // Reinhard tonemapping so HDR irradiance values map to visible range
    irr = irr / (irr + float3(1.0f, 1.0f, 1.0f));
    return float4(irr, 1.0f);
}
