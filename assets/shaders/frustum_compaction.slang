struct VkDrawIndexedIndirectCommand
{
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    int  vertexOffset;
    uint firstInstance;
};

[[vk::binding(2, 0)]] RWStructuredBuffer<VkDrawIndexedIndirectCommand> indirectDrawBuffer;

[[vk::binding(4, 0)]] RWStructuredBuffer<VkDrawIndexedIndirectCommand> compactedDrawBuffer;

[[vk::binding(5, 0)]] RWStructuredBuffer<uint> drawCountBuffer;

struct PushConstants
{
    uint drawCommandCount;
    uint outputOffset;
    uint inputOffset;
    uint countIndex;
};
[[vk::push_constant]] PushConstants push;


[shader("compute")]
[numthreads(64, 1, 1)]
void computeMain(uint3 id : SV_DispatchThreadID)
{
    uint drawIndex = id.x;
    if (drawIndex >= push.drawCommandCount)
        return;

    VkDrawIndexedIndirectCommand cmd = indirectDrawBuffer[drawIndex + push.inputOffset];

    if (cmd.instanceCount > 0)
    {
        uint slotIndex;
        InterlockedAdd(drawCountBuffer[push.countIndex], 1, slotIndex);
        compactedDrawBuffer[slotIndex + push.outputOffset] = cmd;
    }
}