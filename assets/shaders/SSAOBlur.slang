[vk::binding(0, 0)] Sampler2D ssaoTexture;
[vk::binding(1, 0)] Sampler2D unused1;
[vk::binding(2, 0)] Sampler2D unused2;

struct PushConstants
{
    float2 texelSize; // 1.0 / resolution
};
[[vk::push_constant]] ConstantBuffer<PushConstants> pc;

struct VSOutput
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD0;
};

[shader("vertex")]
VSOutput vertMain(uint vertexID : SV_VertexID)
{
    VSOutput output;
    output.uv  = float2((vertexID << 1) & 2, vertexID & 2);
    output.pos = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target
{
    float result = 0.0;

    // 4x4 box blur
    [unroll]
    for (int x = -2; x < 2; x++)
    {
        [unroll]
        for (int y = -2; y < 2; y++)
        {
            float2 offset = float2(float(x), float(y)) * pc.texelSize;
            result += ssaoTexture.SampleLevel(input.uv + offset, 0).r;
        }
    }

    result /= 16.0;
    return float4(result, result, result, 1.0);
}
