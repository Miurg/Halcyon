#include "Shared/Bindings.h"

import Common.BRDF;
import Common.IBL;
import Common.Constants;

[[vk::binding(BIND_TEXTURES_CUBEMAP_SAMPLER, 0)]]
SamplerCube environmentMap;
[[vk::binding(BIND_TEXTURES_CUBEMAP_STORAGE, 0)]]
RWTexture2DArray<float4> outPrefiltered;

struct PushConstants
{
	float roughness;
};

[[vk::push_constant]]
PushConstants pushConstants;

// Van der Corput radical inverse

// GGX importance sampling

[shader("compute")]
[numthreads(8, 8, 1)]
void computeMain(uint3 threadId: SV_DispatchThreadID)
{
	uint width, height, elements;
	outPrefiltered.GetDimensions(width, height, elements);

	if (threadId.x >= width || threadId.y >= height) return;

	float2 uv = float2(threadId.xy) / float2(width, height);
	uv = uv * 2.0 - 1.0;

	float3 N = float3(0, 0, 0);
	switch (threadId.z)
	{
	case 0: N = float3( 1.0, -uv.y, -uv.x); break;
	case 1: N = float3(-1.0, -uv.y,  uv.x); break;
	case 2: N = float3( uv.x,  1.0,  uv.y); break;
	case 3: N = float3( uv.x, -1.0, -uv.y); break;
	case 4: N = float3( uv.x, -uv.y,  1.0); break;
	case 5: N = float3(-uv.x, -uv.y, -1.0); break;
	}
	N = normalize(N);

	// assume V == N (reflection direction == normal)
	float3 R = N;
	float3 V = R;

	float roughness = pushConstants.roughness;
	float totalWeight = 0.0;
	float3 prefilteredColor = float3(0.0, 0.0, 0.0);

	for (uint i = 0u; i < SAMPLE_COUNT; i++)
	{
		float2 Xi = Hammersley(i, SAMPLE_COUNT);
		float3 H = ImportanceSampleGGX(Xi, N, roughness);
		float3 L = normalize(2.0 * dot(V, H) * H - V);

		float NdotL = max(dot(N, L), 0.0);
		if (NdotL > 0.0)
		{
			prefilteredColor += environmentMap.SampleLevel(L, 0).rgb * NdotL;
			totalWeight += NdotL;
		}
	}

	prefilteredColor = prefilteredColor / totalWeight;
	outPrefiltered[threadId] = float4(prefilteredColor, 1.0);
}
