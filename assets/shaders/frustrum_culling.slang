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
[[vk::binding(0, 1)]] RWStructuredBuffer<VkDrawIndexedIndirectCommand> indirectDrawBuffer;

[[vk::binding(1, 1)]] RWStructuredBuffer<uint> visibleIndicesBuffer;

struct PushConstants
{
    uint objectCount;
    uint drawCommandIndex;
};

[[vk::push_constant]] PushConstants push;

// --- Хелпер для проверки AABB ---

// Проверка: находится ли AABB полностью вне одной из плоскостей отсечения
bool IsAABBVisible(float4x4 mvp, float3 minPos, float3 maxPos)
{
    // 8 углов AABB
    float3 corners[8];
    corners[0] = float3(minPos.x, minPos.y, minPos.z);
    corners[1] = float3(maxPos.x, minPos.y, minPos.z);
    corners[2] = float3(minPos.x, maxPos.y, minPos.z);
    corners[3] = float3(maxPos.x, maxPos.y, minPos.z);
    corners[4] = float3(minPos.x, minPos.y, maxPos.z);
    corners[5] = float3(maxPos.x, minPos.y, maxPos.z);
    corners[6] = float3(minPos.x, maxPos.y, maxPos.z);
    corners[7] = float3(maxPos.x, maxPos.y, maxPos.z);

    // Проверяем все 6 плоскостей клип-спейса
    // В Vulkan клип-спейс: x, y in [-w, w], z in [0, w]
    
    // Флаги, полностью ли бокс "за" плоскостью
    bool allLeft = true;
    bool allRight = true;
    bool allBottom = true;
    bool allTop = true;
    bool allNear = true;
    bool allFar = true;

    for (int i = 0; i < 8; i++)
    {
        // Переводим точку в Clip Space
        float4 clipPos = mul(mvp, float4(corners[i], 1.0));

        // Если хотя бы одна точка ВНУТРИ плоскости, то эту плоскость отбрасываем как разделяющую
        if (clipPos.x >= -clipPos.w) allLeft = false;
        if (clipPos.x <= clipPos.w)  allRight = false;
        if (clipPos.y >= -clipPos.w) allBottom = false;
        if (clipPos.y <= clipPos.w)  allTop = false;
        if (clipPos.z >= 0.0)        allNear = false;
        if (clipPos.z <= clipPos.w)  allFar = false;
    }

    // Если все точки находятся за любой из плоскостей - объект невидим
    if (allLeft || allRight || allBottom || allTop || allNear || allFar)
        return false;

    return true;
}

// --- Main ---

[numthreads(64, 1, 1)]
void computeMain(uint3 id : SV_DispatchThreadID)
{
    uint index = id.x;

    if (index >= push.objectCount)
        return;

    // 1. Получаем данные
    ModelData obj = objectBuffer[index];
    TransformData trans = transformBuffer[obj.transformIndex];
    
    // MVP = Projection * View * Model
    float4x4 mvp = mul(camera[0].cameraSpaceMatrix, trans.model);

    // 2. Проверка видимости
    // Используем xyz из float4 AABB
    if (IsAABBVisible(mvp, obj.AABBMin.xyz, obj.AABBMax.xyz))
    {
        // 3. Атомарное добавление
        uint slotIndex;
        // Увеличиваем instanceCount в структуре IndirectCommand
        InterlockedAdd(indirectDrawBuffer[push.drawCommandIndex].instanceCount, 1, slotIndex);

        // 4. Запись индекса
        visibleIndicesBuffer[slotIndex] = index;
    }
}