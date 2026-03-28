[[vk::binding(0, 0)]]
Sampler2D inputTexture;

struct PushConstants
{
    float2 texelSize;
    float threshold;
    float knee;
    int isFirstPass; // 1 = use threshold
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

float3 ApplyThreshold(float3 color, float threshold, float knee)
{
    float brightness = max(color.r, max(color.g, color.b));
    float rq = clamp(brightness - threshold + knee, 0.0, 2.0 * knee);
    rq = (rq * rq) / (4.0 * knee + 1e-4);
    float weight = max(rq, brightness - threshold) / max(brightness, 1e-4);
    return color * weight;
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target
{
    float2 ts = pc.texelSize;

    float3 a = inputTexture.SampleLevel(input.uv + float2(-2, -2) * ts, 0).rgb;
    float3 b = inputTexture.SampleLevel(input.uv + float2( 0, -2) * ts, 0).rgb;
    float3 c = inputTexture.SampleLevel(input.uv + float2( 2, -2) * ts, 0).rgb;
    float3 d = inputTexture.SampleLevel(input.uv + float2(-1, -1) * ts, 0).rgb;
    float3 e = inputTexture.SampleLevel(input.uv + float2( 1, -1) * ts, 0).rgb;
    float3 f = inputTexture.SampleLevel(input.uv + float2(-2,  0) * ts, 0).rgb;
    float3 g = inputTexture.SampleLevel(input.uv + float2( 0,  0) * ts, 0).rgb;
    float3 h = inputTexture.SampleLevel(input.uv + float2( 2,  0) * ts, 0).rgb;
    float3 i = inputTexture.SampleLevel(input.uv + float2(-1,  1) * ts, 0).rgb;
    float3 j = inputTexture.SampleLevel(input.uv + float2( 1,  1) * ts, 0).rgb;
    float3 k = inputTexture.SampleLevel(input.uv + float2(-2,  2) * ts, 0).rgb;
    float3 l = inputTexture.SampleLevel(input.uv + float2( 0,  2) * ts, 0).rgb;
    float3 m = inputTexture.SampleLevel(input.uv + float2( 2,  2) * ts, 0).rgb;

    float3 result = (d + e + i + j) * 0.5;
    result += (a + b + g + f) * 0.125;
    result += (b + c + h + g) * 0.125;
    result += (f + g + l + k) * 0.125;
    result += (g + h + m + l) * 0.125;

    if (pc.isFirstPass == 1)
        result = ApplyThreshold(result, pc.threshold, pc.knee);

    return float4(result, 1.0);
}

// ========================================================================================
// Method: Jimenez Bloom / Dual-Kawase (13-tap Downsample)
// Author: Jorge Jimenez
// First used in: Call of Duty: Advanced Warfare (2014)
//
// Why 13 taps: A wide sampling pattern is required when halving the resolution to avoid
// losing bright sub-pixel details (like 1-pixel highlights). This effectively prevents
// temporal aliasing (flickering and sparkling) of bright spots during camera movement.
// ========================================================================================