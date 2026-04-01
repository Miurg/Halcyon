[vk::binding(0, 0)]
Sampler2D sourceTexture;

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
    float4 mainColors = sourceTexture.SampleLevel(input.uv, 0.0);
    float distanceToCenter = distance(input.uv, float2(0.5, 0.5));
    float4 vignetteColor = mainColors * smoothstep(0.9, 0.1, distanceToCenter);
    return vignetteColor;
}
