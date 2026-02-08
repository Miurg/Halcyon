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
    float3 corners[8];
    corners[0] = float3(minPos.x, minPos.y, minPos.z);
    corners[1] = float3(maxPos.x, minPos.y, minPos.z);
    corners[2] = float3(minPos.x, maxPos.y, minPos.z);
    corners[3] = float3(maxPos.x, maxPos.y, minPos.z);
    corners[4] = float3(minPos.x, minPos.y, maxPos.z);
    corners[5] = float3(maxPos.x, minPos.y, maxPos.z);
    corners[6] = float3(minPos.x, maxPos.y, maxPos.z);
    corners[7] = float3(maxPos.x, maxPos.y, maxPos.z);

    bool allLeft = true;
    bool allRight = true;
    bool allBottom = true;
    bool allTop = true;
    bool allNear = true;
    bool allFar = true;

    // Check each corner against the frustum planes
    for (int i = 0; i < 8; i++)
    {
        float4 clipPos = mul(mvp, float4(corners[i], 1.0));

        if (clipPos.x >= -clipPos.w) allLeft = false;
        if (clipPos.x <= clipPos.w)  allRight = false;
        if (clipPos.y >= -clipPos.w) allBottom = false;
        if (clipPos.y <= clipPos.w)  allTop = false;
        if (clipPos.z >= 0.0)        allNear = false;
        if (clipPos.z <= clipPos.w)  allFar = false;
    }

    if (allLeft || allRight || allBottom || allTop || allNear || allFar)
        return false;

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