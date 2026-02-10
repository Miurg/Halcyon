struct ModelData
{
    float4 colorFactor;
    float4 AABBMin;
    float4 AABBMax;
    uint transformIndex;
    uint textureIndex;
    float2 padding;
};
[[vk::binding(0, 2)]] StructuredBuffer<ModelData> objectBuffer;

struct TransformData
{
    float4x4 model;
};
[[vk::binding(1, 2)]] StructuredBuffer<TransformData> transformBuffer;

struct CameraData
{
    float4x4 cameraSpaceMatrix; // View * Projection
};
[[vk::binding(0, 0)]] StructuredBuffer<CameraData> camera;

struct VkDrawIndexedIndirectCommand
{
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    int vertexOffset;
    uint firstInstance;
};
[[vk::binding(0, 3)]] RWStructuredBuffer<VkDrawIndexedIndirectCommand> indirectDrawBuffer;

[[vk::binding(1, 3)]] RWStructuredBuffer<uint> visibleIndicesBuffer;

struct PushConstants
{
    uint objectCount;
    uint drawCommandIndex;
    uint visibleBufferOffset;
};

[[vk::push_constant]] PushConstants push;

// Helper function to check if an AABB is visible in the view frustum
bool IsAABBVisible(float4x4 mvp, float3 minPos, float3 maxPos)
{
    float4 planes[6];
    // Left:   x >= -w  -> x + w >= 0
    planes[0] = mvp[3] + mvp[0];
    // Right:  x <= w   -> w - x >= 0
    planes[1] = mvp[3] - mvp[0];
    // Bottom: y >= -w  -> y + w >= 0
    planes[2] = mvp[3] + mvp[1];
    // Top:    y <= w   -> w - y >= 0
    planes[3] = mvp[3] - mvp[1];
    // Near:   z >= 0   -> z >= 0 Vulkan specifies that the near plane is at z=0 in clip space
    planes[4] = mvp[2];
    // Far:    z <= w   -> w - z >= 0
    planes[5] = mvp[3] - mvp[2];

    [unroll]
    for (int i = 0; i < 6; i++)
    {
        float3 p;
        p.x = (planes[i].x > 0) ? maxPos.x : minPos.x;
        p.y = (planes[i].y > 0) ? maxPos.y : minPos.y;
        p.z = (planes[i].z > 0) ? maxPos.z : minPos.z;

        // dot(plane, float4(p, 1.0)) < 0
        if (dot(planes[i].xyz, p) + planes[i].w < 0)
        {
            return false;
        }
    }

    return true;
}

// --- Main ---

[shader("compute")]
[numthreads(64, 1, 1)]
void computeMain(uint3 id : SV_DispatchThreadID)
{
    uint index = id.x;
    if (index >= push.objectCount) // Bounds check to avoid out-of-range access
        return;

    ModelData obj = objectBuffer[index + push.visibleBufferOffset];
    TransformData trans = transformBuffer[obj.transformIndex];

    float4x4 mvp = mul(camera[0].cameraSpaceMatrix, trans.model);

    if (IsAABBVisible(mvp, obj.AABBMin.xyz, obj.AABBMax.xyz))
    {
        // Atomically increment the instance count 
        uint slotIndex;
        InterlockedAdd(indirectDrawBuffer[push.drawCommandIndex].instanceCount, 1, slotIndex);

        visibleIndicesBuffer[slotIndex + push.visibleBufferOffset] = index + push.visibleBufferOffset;
    }
}