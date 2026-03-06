struct VkDrawIndexedIndirectCommand
{
	uint indexCount;
	uint instanceCount;
	uint firstIndex;
	int vertexOffset;
	uint firstInstance;
};

[[vk::binding(2, 0)]]
RWStructuredBuffer<VkDrawIndexedIndirectCommand> indirectDrawBuffer;

struct PushConstants
{
	uint drawCommandCount;
};
[[vk::push_constant]]
PushConstants push;

[shader("compute")]
[numthreads(64, 1, 1)]
void computeMain(uint3 id: SV_DispatchThreadID)
{
	uint drawIndex = id.x;
	if (drawIndex >= push.drawCommandCount) return;

	indirectDrawBuffer[drawIndex].instanceCount = 0;
}
