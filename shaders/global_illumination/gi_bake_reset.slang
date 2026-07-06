struct VkDrawIndexedIndirectCommand
{
	uint indexCount;
	uint instanceCount;
	uint firstIndex;
	int vertexOffset;
	uint firstInstance;
};

// Set 0 = main modelSet (draw command templates), set 1 = bake modelSet (per-region outputs)
[[vk::binding(2, 0)]]
StructuredBuffer<VkDrawIndexedIndirectCommand> templateDrawBuffer;

[[vk::binding(2, 1)]]
RWStructuredBuffer<VkDrawIndexedIndirectCommand> bakeIndirectDrawBuffer;

[[vk::binding(5, 1)]]
RWStructuredBuffer<uint> bakeDrawCountBuffer;

struct PushConstants
{
	uint drawCommandCount;
	uint visibleCapacity;
};
[[vk::push_constant]]
PushConstants push;

[shader("compute")]
[numthreads(64, 1, 1)]
void computeMain(uint3 id: SV_DispatchThreadID)
{
	uint cmdIndex = id.x;
	uint region = id.y;

	if (cmdIndex < 6) bakeDrawCountBuffer[region * 6 + cmdIndex] = 0;
	if (cmdIndex >= push.drawCommandCount) return;

	VkDrawIndexedIndirectCommand cmd = templateDrawBuffer[cmdIndex];
	cmd.instanceCount = 0;
	cmd.firstInstance += region * push.visibleCapacity;
	bakeIndirectDrawBuffer[region * push.drawCommandCount + cmdIndex] = cmd;
}
