#include "Shared/GpuStructs.h"

[[vk::binding(2, 0)]]
RWStructuredBuffer<IndirectDrawIndexedCommand> indirectDrawBuffer;

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
