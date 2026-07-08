// === SET 0 (globalSet) ===
struct SHGridInfo
{
    float3 origin; float spacing;
    int3 count;    uint probeCount;
    float captureRange; float3 giAmbient;
};
[[vk::binding(5, 0)]]
StructuredBuffer<SHGridInfo> shGridInfo;

// === SET 1 (bake modelSet) ===
struct ModelData
{
	float4 AABBMin;
	float4 AABBMax;
	uint transformIndex;
	uint materialIndex;
	uint drawCommandIndex;
};
[[vk::binding(0, 1)]]
StructuredBuffer<ModelData> objectBuffer;

struct TransformData
{
	float4x4 model;
};
[[vk::binding(1, 1)]]
StructuredBuffer<TransformData> transformBuffer;

struct VkDrawIndexedIndirectCommand
{
	uint indexCount;
	uint instanceCount;
	uint firstIndex;
	int vertexOffset;
	uint firstInstance;
};
[[vk::binding(2, 1)]]
RWStructuredBuffer<VkDrawIndexedIndirectCommand> bakeIndirectDrawBuffer;

[[vk::binding(3, 1)]]
RWStructuredBuffer<uint> bakeVisibleIndicesBuffer;

struct PushConstants
{
	uint objectCount;
	uint drawCommandCount;
	uint firstProbeLinear;
};
[[vk::push_constant]]
PushConstants push;

// Must match the face tables in the bake render shaders and faceDirection() in sh_projection.slang
static const float3 kFaceForward[6] = {
	float3(1, 0, 0), float3(-1, 0, 0), float3(0, 1, 0), float3(0, -1, 0), float3(0, 0, 1), float3(0, 0, -1),
};
static const float3 kFaceUp[6] = {
	float3(0, 1, 0), float3(0, 1, 0), float3(0, 0, -1), float3(0, 0, 1), float3(0, 1, 0), float3(0, 1, 0),
};
// Reversed-Z: near/far swapped, must match bakeFaceClip() in the bake render shaders
static const float kBakeFar = 0.01f;

void faceFrustumPlanes(float3 probePos, int face, out float4 planes[6])
{
	float3 f = kFaceForward[face];
	float3 s = normalize(cross(f, kFaceUp[face]));
	float3 u = cross(s, f);

	// b = -(zFar*zNear)/(zFar-zNear), glm sign convention: positive with swapped reversed-Z near/far
	const float range = shGridInfo[0].captureRange;
	const float a = kBakeFar / (range - kBakeFar);
	const float b = (kBakeFar * range) / (range - kBakeFar);

	// Row vectors of P*V for the analytic face camera (see bakeFaceClip in the render shaders)
	float4 row0 = float4(s, -dot(s, probePos));
	float4 row1 = float4(-u, dot(u, probePos));
	float4 row2 = float4(-a * f, a * dot(f, probePos) + b);
	float4 row3 = float4(f, -dot(f, probePos));

	// Same Gribb-Hartmann extraction as CameraMatrixSystem (normalization skipped: sign test only)
	planes[0] = row3 + row0;
	planes[1] = row3 - row0;
	planes[2] = row3 + row1;
	planes[3] = row3 - row1;
	planes[4] = row2;
	planes[5] = row3 - row2;
}

bool IsAABBVisible(float4 planes[6], float3 minPos, float3 maxPos)
{
	[unroll]
	for (int i = 0; i < 6; i++)
	{
		// Pick the positive vertex for this plane
		float3 p;
		p.x = (planes[i].x > 0) ? maxPos.x : minPos.x;
		p.y = (planes[i].y > 0) ? maxPos.y : minPos.y;
		p.z = (planes[i].z > 0) ? maxPos.z : minPos.z;

		// If the positive vertex is outside, the whole AABB is outside
		if (dot(planes[i].xyz, p) + planes[i].w < 0) return false;
	}

	return true;
}

[shader("compute")]
[numthreads(64, 1, 1)]
void computeMain(uint3 id: SV_DispatchThreadID)
{
	uint index = id.x;
	uint region = id.y;
	if (index >= push.objectCount) return;

	SHGridInfo grid = shGridInfo[0];
	int probeLinear = int(push.firstProbeLinear + region / 6);
	int face = int(region % 6);

	int ix = probeLinear % grid.count.x;
	int iy = (probeLinear / grid.count.x) % grid.count.y;
	int iz = probeLinear / (grid.count.x * grid.count.y);
	float3 probePos = grid.origin + grid.spacing * float3(ix, iy, iz);

	float4 planes[6];
	faceFrustumPlanes(probePos, face, planes);

	ModelData obj = objectBuffer[index];
	TransformData trans = transformBuffer[obj.transformIndex];

	float3 localCenter = (obj.AABBMax.xyz + obj.AABBMin.xyz) * 0.5;
	float3 localExtents = (obj.AABBMax.xyz - obj.AABBMin.xyz) * 0.5;

	float3 worldCenter = mul(trans.model, float4(localCenter, 1.0)).xyz;
	float3 right = abs(mul((float3x3)trans.model, float3(1, 0, 0))) * localExtents.x;
	float3 up = abs(mul((float3x3)trans.model, float3(0, 1, 0))) * localExtents.y;
	float3 forward = abs(mul((float3x3)trans.model, float3(0, 0, 1))) * localExtents.z;
	float3 worldExtents = right + up + forward;

	if (!IsAABBVisible(planes, worldCenter - worldExtents, worldCenter + worldExtents)) return;

	uint cmdSlot = region * push.drawCommandCount + obj.drawCommandIndex;
	uint visibleBufferOffset = bakeIndirectDrawBuffer[cmdSlot].firstInstance;

	uint slotIndex;
	InterlockedAdd(bakeIndirectDrawBuffer[cmdSlot].instanceCount, 1, slotIndex);

	bakeVisibleIndicesBuffer[slotIndex + visibleBufferOffset] = index;
}
