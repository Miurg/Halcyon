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

// TODO: Add Reinhard, Uncharted 2, Hable, etc. tone mapping operators for comparison.

// === Tone Mapping ===
// ACES Filmic (Narkowicz 2015)
float3 ACESFilm(float3 x)
{
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;
	return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

// All this stuff below doesn't work like it should. 
// Maybe I messed up the matrices, or maybe it's just not a good fit for real-time rendering.
// Keep for future. 

// === AgX Tone Mapping ===
// Based on Troy Lim's AgX, GLSL reference by bwrensch
static const float3x3 AgXInputMatrix = float3x3(
	0.856627153315983, 0.137318972929847, 0.11189821299995,
	0.0951212405381588, 0.761241990602591, 0.0767994186031903,
	0.0482516061458583, 0.101439036467562, 0.811302368396859
);

static const float3x3 AgXOutputMatrix = float3x3(
	 1.1271005818144368, -0.1413297634984383, -0.14132976349843826,
	-0.11060664309660323, 1.157823702216272, -0.11060664309660294,
	-0.01647326386032981, -0.016473263860329533, 1.2519364065950405
);
float3 AgXDefaultContrastApprox(float3 x)
{
    float3 x2 = x * x;
    float3 x4 = x2 * x2;
    return +15.5 * x4 * x2
	       - 40.14 * x4 * x
	       + 31.96 * x4
	       - 6.868 * x2 * x
	       + 0.4298 * x2
	       + 0.1191 * x
	       - 0.00232;
}

float3 AgX(float3 color)
{
    color = max(color, 0.0);

    color = mul(AgXInputMatrix, color);

    const float minEV = -10.0;
    const float maxEV = 6.5;
    color = clamp(log2(color), minEV, maxEV);
    color = (color - minEV) / (maxEV - minEV); 

    color = AgXDefaultContrastApprox(color);

    color = mul(AgXOutputMatrix, color);

    return color;
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target
{
    float4 hdr = sourceTexture.SampleLevel(input.uv, 0.0);
    float3 mapped = ACESFilm(hdr.rgb);

	return float4(mapped, 1.0);
}
