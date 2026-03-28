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
    float centerSsao = ssaoTexture.SampleLevel(input.uv, 0).r;
    float centerDepth = depthTexture.SampleLevel(input.uv, 0).r;

    // Dont blur background
    if (centerDepth <= 0.0) return 1.0;

    float3 centerNormal = normalsTexture.SampleLevel(input.uv, 0).xyz;

    float combinedSsao = 0.0;
    float totalWeight = 0.0;

    const float weights[7] = { 0.006, 0.061, 0.242, 0.383, 0.242, 0.061, 0.006 };

    for (int x = -3; x <= 3; x++)
    {
        for (int y = -3; y <= 3; y++)
        {
            float2 offset = float2(x, y) * pc.texelSize;
            float2 sampleUV = input.uv + offset;

            float sampleDepth = depthTexture.SampleLevel(sampleUV, 0).r;
            float3 sampleNormal = normalsTexture.SampleLevel(sampleUV, 0).xyz;
            float sampleSsao = ssaoTexture.SampleLevel(sampleUV, 0).r;

            float depthDiff = abs(centerDepth - sampleDepth);
            float depthWeight = exp(-depthDiff * 100.0);

            float normalDiff = max(dot(centerNormal, sampleNormal), 0.0);
            float normalWeight = pow(normalDiff, 16.0); 

            float spatialWeight = weights[x + 3] * weights[y + 3];
            float finalWeight = spatialWeight * depthWeight * normalWeight;

            combinedSsao += sampleSsao * finalWeight;
            totalWeight += finalWeight;
        }
    }

    float result = (totalWeight > 0.0001) ? (combinedSsao / totalWeight) : centerSsao;

    return float4(result.xxx, 1.0);
}