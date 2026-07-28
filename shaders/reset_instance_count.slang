#include "Shared/GpuStructs.h"
#include "Shared/Bindings.h"

[[vk::binding(BIND_MODEL_INDIRECT_DRAW, 0)]]
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
