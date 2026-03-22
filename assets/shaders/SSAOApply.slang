[vk::binding(0, 0)]
Sampler2D sourceTexture;
[vk::binding(1, 0)]
Sampler2D ssaoTexture;

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
	float3 mapped = sourceTexture.SampleLevel(input.uv, 0).rgb;

	float ao = ssaoTexture.SampleLevel(input.uv, 0).r;
	mapped *= ao;

	return float4(mapped, 1.0);
}
