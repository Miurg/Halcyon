#include "Shared/GpuStructs.h"
#include "Shared/Bindings.h"

// Set 0 = main modelSet (draw command templates), set 1 = bake modelSet (per-region outputs)
[[vk::binding(BIND_MODEL_INDIRECT_DRAW, 0)]]
StructuredBuffer<IndirectDrawIndexedCommand> templateDrawBuffer;

[[vk::binding(BIND_MODEL_INDIRECT_DRAW, 1)]]
RWStructuredBuffer<IndirectDrawIndexedCommand> bakeIndirectDrawBuffer;

[[vk::binding(BIND_MODEL_DRAW_COUNT, 1)]]
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

	IndirectDrawIndexedCommand cmd = templateDrawBuffer[cmdIndex];
	cmd.instanceCount = 0;
	cmd.firstInstance += region * push.visibleCapacity;
	bakeIndirectDrawBuffer[region * push.drawCommandCount + cmdIndex] = cmd;
}
