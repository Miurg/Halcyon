struct VkDrawIndexedIndirectCommand
{
	uint indexCount;
	uint instanceCount;
	uint firstIndex;
	int vertexOffset;
	uint firstInstance;
};

// Set 0 = bake modelSet
[[vk::binding(2, 0)]]
RWStructuredBuffer<VkDrawIndexedIndirectCommand> bakeIndirectDrawBuffer;

[[vk::binding(4, 0)]]
RWStructuredBuffer<VkDrawIndexedIndirectCommand> bakeCompactedDrawBuffer;

[[vk::binding(5, 0)]]
RWStructuredBuffer<uint> bakeDrawCountBuffer;

struct PushConstants
{
	uint drawCommandCount;
	uint segStart1;
	uint segStart2;
	uint segStart3;
	uint segStart4;
	uint segStart5;
};
[[vk::push_constant]]
PushConstants push;

[shader("compute")]
[numthreads(64, 1, 1)]
void computeMain(uint3 id: SV_DispatchThreadID)
{
	uint cmdIndex = id.x;
	uint region = id.y;
	if (cmdIndex >= push.drawCommandCount) return;

	VkDrawIndexedIndirectCommand cmd = bakeIndirectDrawBuffer[region * push.drawCommandCount + cmdIndex];
	if (cmd.instanceCount == 0) return;

	uint segStarts[6] = { 0, push.segStart1, push.segStart2, push.segStart3, push.segStart4, push.segStart5 };
	uint segment = 0;
	[unroll]
	for (int s = 1; s < 6; ++s)
		if (cmdIndex >= segStarts[s]) segment = uint(s);

	uint slotIndex;
	InterlockedAdd(bakeDrawCountBuffer[region * 6 + segment], 1, slotIndex);
	bakeCompactedDrawBuffer[region * push.drawCommandCount + segStarts[segment] + slotIndex] = cmd;
}
