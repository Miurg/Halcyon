#include "Shared/GpuStructs.h"
#include "Shared/Bindings.h"

[[vk::binding(BIND_MODEL_INDIRECT_DRAW, 0)]]
RWStructuredBuffer<IndirectDrawIndexedCommand> indirectDrawBuffer;

[[vk::binding(BIND_MODEL_COMPACTED_DRAW, 0)]]
RWStructuredBuffer<IndirectDrawIndexedCommand> compactedDrawBuffer;

[[vk::binding(BIND_MODEL_DRAW_COUNT, 0)]]
RWStructuredBuffer<uint> drawCountBuffer;

struct PushConstants
{
	uint drawCommandCount;
	uint outputOffset;
	uint inputOffset;
	uint countIndex;
};
[[vk::push_constant]]
PushConstants push;

[shader("compute")]
[numthreads(64, 1, 1)]
void computeMain(uint3 id: SV_DispatchThreadID)
{
	uint drawIndex = id.x;
	if (drawIndex >= push.drawCommandCount) return;

	IndirectDrawIndexedCommand cmd = indirectDrawBuffer[drawIndex + push.inputOffset];

	if (cmd.instanceCount > 0)
	{
		uint slotIndex;
		InterlockedAdd(drawCountBuffer[push.countIndex], 1, slotIndex);
		compactedDrawBuffer[slotIndex + push.outputOffset] = cmd;
	}
}
