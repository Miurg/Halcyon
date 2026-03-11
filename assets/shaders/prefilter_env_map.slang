[[vk::binding(3, 0)]]
SamplerCube environmentMap;
[[vk::binding(4, 0)]]
RWTexture2DArray<float4> outPrefiltered;

static const float PI = 3.14159265359f;
static const uint SAMPLE_COUNT = 1024u;

struct PushConstants
{
	float roughness;
};

[[vk::push_constant]]
PushConstants pushConstants;

// Van der Corput radical inverse
float RadicalInverse_VdC(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10;
}

float2 Hammersley(uint i, uint N)
{
	return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

// GGX importance sampling
float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{
	float a = roughness * roughness;

	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	// Spherical to cartesian
	float3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;

	// Tangent to world
	float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
	float3 tangent = normalize(cross(up, N));
	float3 bitangent = cross(N, tangent);

	float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}

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
