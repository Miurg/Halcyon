[[vk::binding(0, 0)]]
Sampler2D ssaoTexture;
[[vk::binding(1, 0)]]
Sampler2D depthTexture;
[[vk::binding(2, 0)]]
Sampler2D normalsTexture;

struct PushConstants
{
	float2 texelSize; // 1.0 / resolution
};
[[vk::push_constant]]
ConstantBuffer<PushConstants> pc;

struct VSOutput
{
	float4 pos : SV_Position;
	float2 uv : TEXCOORD0;
};

[shader("vertex")]
VSOutput vertMain(uint vertexID: SV_VertexID)
{
	VSOutput output;
	output.uv = float2((vertexID << 1) & 2, vertexID & 2);
	output.pos = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
	return output;
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target
{
	float result = 0.0;
	float totalWeight = 0.0;

	// Center pixel data for bilateral blur
	float centerDepth = depthTexture.SampleLevel(input.uv, 0).r;
	if (centerDepth <= 0.0) return float4(1.0, 1.0, 1.0, 1.0);
	float3 centerNormal = normalsTexture.SampleLevel(input.uv, 0).xyz;

	const float weights[7] = { 0.006, 0.061, 0.242, 0.383, 0.242, 0.061, 0.006 };

	[unroll]
	for (int x = -3; x <= 3; x++)
	{
		[unroll]
		for (int y = -3; y <= 3; y++)
		{
			float2 offset = float2(float(x), float(y)) * pc.texelSize;
			float2 sampleUV = input.uv + offset;

			float sampleDepth = depthTexture.SampleLevel(sampleUV, 0).r;
			float3 sampleNormal = normalsTexture.SampleLevel(sampleUV, 0).xyz;
			float sampleSsao = ssaoTexture.SampleLevel(sampleUV, 0).r;

			// Bilateral weights: Depth and Normal difference
			// Tuning parameters to reject samples that are on different surfaces
			float depthDiff = abs(centerDepth - sampleDepth);
			float depthWeight = exp(-depthDiff * 1000.0); // Sensitive to small depth changes 

			float normalDiff = 1.0 - max(dot(centerNormal, sampleNormal), 0.0);
			float normalWeight = exp(-normalDiff * 10.0);

			float spatialWeight = weights[x + 3] * weights[y + 3];

			float finalWeight = spatialWeight * depthWeight * normalWeight;

			result += sampleSsao * finalWeight;
			totalWeight += finalWeight;
		}
	}

	result /= max(totalWeight, 0.001); // Prevent division by zero

	return float4(result, result, result, 1.0);
}
