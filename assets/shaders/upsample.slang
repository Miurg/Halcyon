[[vk::binding(0, 0)]]
Sampler2D currentTexture;
[[vk::binding(1, 0)]]
Sampler2D previousTexture;

struct PushConstants
{
    float2 texelSize;
    float blendFactor;
    int isLastPass; // 1 = mix with MainColor
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
    float2 ts = pc.texelSize;

    // 9-tap tent filter
    float3 a = currentTexture.SampleLevel(input.uv + float2(-1, -1) * ts, 0).rgb;
    float3 b = currentTexture.SampleLevel(input.uv + float2( 0, -1) * ts, 0).rgb;
    float3 c = currentTexture.SampleLevel(input.uv + float2( 1, -1) * ts, 0).rgb;
    float3 d = currentTexture.SampleLevel(input.uv + float2(-1,  0) * ts, 0).rgb;
    float3 e = currentTexture.SampleLevel(input.uv + float2( 0,  0) * ts, 0).rgb;
    float3 f = currentTexture.SampleLevel(input.uv + float2( 1,  0) * ts, 0).rgb;
    float3 g = currentTexture.SampleLevel(input.uv + float2(-1,  1) * ts, 0).rgb;
    float3 h = currentTexture.SampleLevel(input.uv + float2( 0,  1) * ts, 0).rgb;
    float3 i = currentTexture.SampleLevel(input.uv + float2( 1,  1) * ts, 0).rgb;

    float3 upsampled = (a + c + g + i) * 0.0625
                     + (b + d + f + h) * 0.125
                     + e * 0.25;

    float3 prev = previousTexture.SampleLevel(input.uv, 0).rgb;
    float3 result = lerp(prev, prev + upsampled, pc.blendFactor);

    return float4(result, 1.0);
}

// ========================================================================================
// Method: Jimenez Bloom / Dual-Kawase (9-tap Tent Upsample)
// Author: Jorge Jimenez
// First used in: Call of Duty: Advanced Warfare (2014)
//
// Why 9 taps: The image from the lower mip level is already heavily blurred and lacks
// sharp details. A 9-tap (3x3) tent filter provides a cheap and smooth upscaling of
// this soft data, preventing blocky or square interpolation artifacts.
// ========================================================================================