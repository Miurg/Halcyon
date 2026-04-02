struct CameraData
{
    float4x4 cameraSpaceMatrix;
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float4x4 invViewProj;
    float4   cameraPositionAndPadding;
    float4   frustumPlanes[6];
};
[[vk::binding(0, 0)]] StructuredBuffer<CameraData> camera;

struct PushConstants
{
    float4x4 model;
    float3   aabbMin;
    float    pad0;
    float3   aabbMax;
    float    pad1;
};
[[vk::push_constant]] ConstantBuffer<PushConstants> pc;

// 12 edges × 2 endpoints = 24 vertices
// Each entry: [cornerA_bits, cornerB_bits], bit0=x, bit1=y, bit2=z selects min/max
static const uint2 edges[12] = {
    uint2(0, 1), uint2(2, 3), uint2(4, 5), uint2(6, 7), // along X
    uint2(0, 2), uint2(1, 3), uint2(4, 6), uint2(5, 7), // along Y
    uint2(0, 4), uint2(1, 5), uint2(2, 6), uint2(3, 7), // along Z
};

float3 cornerFromBits(uint b)
{
    return float3(
        (b & 1) ? pc.aabbMax.x : pc.aabbMin.x,
        (b & 2) ? pc.aabbMax.y : pc.aabbMin.y,
        (b & 4) ? pc.aabbMax.z : pc.aabbMin.z
    );
}

[shader("vertex")]
float4 vertMain(uint vid : SV_VertexID) : SV_Position
{
    uint cornerBits = (vid % 2 == 0) ? edges[vid / 2].x : edges[vid / 2].y;
    float4 worldPos = mul(pc.model, float4(cornerFromBits(cornerBits), 1.0));
    return mul(camera[0].cameraSpaceMatrix, worldPos);
}

[shader("fragment")]
float4 fragMain() : SV_Target
{
    return float4(1.0, 0.65, 0.0, 1.0); // orange
}
