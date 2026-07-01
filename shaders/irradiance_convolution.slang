[[vk::binding(3, 0)]]
SamplerCube environmentMap;
[[vk::binding(4, 0)]]
RWTexture2DArray<float4> outIrradiance;

static const float PI = 3.14159265359f;

struct PushConstants
{
	float sampleDelta;
};

[[vk::push_constant]]
PushConstants pushConstants;

[shader("compute")]
[numthreads(8, 8, 1)]
void computeMain(uint3 threadId: SV_DispatchThreadID)
{
	uint width, height, elements;
	outIrradiance.GetDimensions(width, height, elements);

	if (threadId.x >= width || threadId.y >= height) return;

	float2 uv = float2(threadId.xy) / float2(width, height);
	uv = uv * 2.0 - 1.0;

	// Cubemap face direction
	float3 N = float3(0, 0, 0);
	switch (threadId.z)
	{
	case 0: N = float3( 1.0, -uv.y, -uv.x); break; // +X
	case 1: N = float3(-1.0, -uv.y,  uv.x); break; // -X
	case 2: N = float3( uv.x,  1.0,  uv.y); break; // +Y
	case 3: N = float3( uv.x, -1.0, -uv.y); break; // -Y
	case 4: N = float3( uv.x, -uv.y,  1.0); break; // +Z
	case 5: N = float3(-uv.x, -uv.y, -1.0); break; // -Z
	}
	N = normalize(N);

	// Build tangent frame
	float3 up = abs(N.y) < 0.999 ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
	float3 right = normalize(cross(up, N));
	up = cross(N, right);

	float3 irradiance = float3(0.0, 0.0, 0.0);
	float sampleCount = 0.0;
	float delta = pushConstants.sampleDelta;

	for (float phi = 0.0; phi < 2.0 * PI; phi += delta)
	{
		for (float theta = 0.0; theta < 0.5 * PI; theta += delta)
		{
			// Spherical to cartesian (in tangent space)
			float sinTheta = sin(theta);
			float cosTheta = cos(theta);
			float3 tangentSample = float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

			// Tangent space to world
			float3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

			irradiance += environmentMap.SampleLevel(sampleVec, 0).rgb * cosTheta * sinTheta;
			sampleCount += 1.0;
		}
	}

	irradiance = PI * irradiance / sampleCount;
	outIrradiance[threadId] = float4(irradiance, 1.0);
}
