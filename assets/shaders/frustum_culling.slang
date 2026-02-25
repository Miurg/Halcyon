// === SET 0 ===

struct CameraData
{
    float4x4 cameraSpaceMatrix; // View * Projection
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float4   cameraPositionAndPadding;
    float4   frustumPlanes[6];
};
[[vk::binding(0, 0)]] StructuredBuffer<CameraData> camera;

// === SET 1 ===

struct ModelData
{
    float4 AABBMin;
    float4 AABBMax;
    uint   transformIndex;
    uint   materialIndex;
    uint   drawCommandIndex; // unused
};
[[vk::binding(0, 1)]] StructuredBuffer<ModelData> objectBuffer;

struct TransformData
{
    float4x4 model;
};
[[vk::binding(1, 1)]] StructuredBuffer<TransformData> transformBuffer;

struct VkDrawIndexedIndirectCommand
{
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    int  vertexOffset;
    uint firstInstance;
};
[[vk::binding(2, 1)]] RWStructuredBuffer<VkDrawIndexedIndirectCommand> indirectDrawBuffer;

[[vk::binding(3, 1)]] RWStructuredBuffer<uint> visibleIndicesBuffer;

struct PushConstants
{
    uint objectCount;
};
[[vk::push_constant]] PushConstants push;


// Returns true if the AABB is at least partially inside the view frustum
bool IsAABBVisible(float4 planes[6], float3 minPos, float3 maxPos)
{
    [unroll]
    for (int i = 0; i < 6; i++)
    {
        // Pick the positive vertex for this plane
        float3 p;
        p.x = (planes[i].x > 0) ? maxPos.x : minPos.x;
        p.y = (planes[i].y > 0) ? maxPos.y : minPos.y;
        p.z = (planes[i].z > 0) ? maxPos.z : minPos.z;

        // If the positive vertex is outside, the whole AABB is outside
        if (dot(planes[i].xyz, p) + planes[i].w < 0)
            return false;
    }

    return true;
}


[shader("compute")]
[numthreads(64, 1, 1)]
void computeMain(uint3 id : SV_DispatchThreadID)
{
    uint index = id.x;
    if (index >= push.objectCount) // Out of range check
        return;

    ModelData     obj       = objectBuffer[index];
    TransformData trans     = transformBuffer[obj.transformIndex];

    float3 localCenter = (obj.AABBMax.xyz + obj.AABBMin.xyz) * 0.5;
    float3 localExtents = (obj.AABBMax.xyz - obj.AABBMin.xyz) * 0.5;

    float3 worldCenter = mul(trans.model, float4(localCenter, 1.0)).xyz;
    float3 right   = abs(mul((float3x3)trans.model, float3(1, 0, 0))) * localExtents.x;
    float3 up      = abs(mul((float3x3)trans.model, float3(0, 1, 0))) * localExtents.y;
    float3 forward = abs(mul((float3x3)trans.model, float3(0, 0, 1))) * localExtents.z;
    float3 worldExtents = right + up + forward;

    float3 worldMin = worldCenter - worldExtents;
    float3 worldMax = worldCenter + worldExtents;

    if (!IsAABBVisible(camera[0].frustumPlanes, worldMin, worldMax))
        return;

    uint drawCommandIndex    = obj.drawCommandIndex;
    uint visibleBufferOffset = indirectDrawBuffer[drawCommandIndex].firstInstance;

    // Atomically
    uint slotIndex;
    InterlockedAdd(indirectDrawBuffer[drawCommandIndex].instanceCount, 1, slotIndex);

    visibleIndicesBuffer[slotIndex + visibleBufferOffset] = index;
}