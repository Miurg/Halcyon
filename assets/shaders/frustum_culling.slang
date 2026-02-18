// === SET 0 ===

struct CameraData
{
    float4x4 cameraSpaceMatrix; // View * Projection
    float3 cameraPosition;
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
bool IsAABBVisible(float4x4 mvp, float3 minPos, float3 maxPos)
{
    float4 planes[6];
    // Left:   x + w >= 0
    planes[0] = mvp[3] + mvp[0];
    // Right:  w - x >= 0
    planes[1] = mvp[3] - mvp[0];
    // Bottom: y + w >= 0
    planes[2] = mvp[3] + mvp[1];
    // Top:    w - y >= 0
    planes[3] = mvp[3] - mvp[1];
    // Near:   z >= 0  (Vulkan: near plane at z=0 in clip space)
    planes[4] = mvp[2];
    // Far:    w - z >= 0
    planes[5] = mvp[3] - mvp[2];

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
    float4x4      mvp       = mul(camera[0].cameraSpaceMatrix, trans.model);

    if (!IsAABBVisible(mvp, obj.AABBMin.xyz, obj.AABBMax.xyz))
        return;

    uint drawCommandIndex    = obj.drawCommandIndex;
    uint visibleBufferOffset = indirectDrawBuffer[drawCommandIndex].firstInstance;

    // Atomically
    uint slotIndex;
    InterlockedAdd(indirectDrawBuffer[drawCommandIndex].instanceCount, 1, slotIndex);

    visibleIndicesBuffer[slotIndex + visibleBufferOffset] = index;
}