[[vk::binding(0, 0)]]
Sampler2D<float> srcTex;

[[vk::binding(1, 0)]]
[format("r32f")]
RWTexture2D<float> dstMip;

struct PushConstants
{
    uint dstWidth;
    uint dstHeight;
    uint passIdx;
};
[[vk::push_constant]]
PushConstants push;

[shader("compute")]
[numthreads(8, 8, 1)]
void computeMain(uint3 id: SV_DispatchThreadID)
{
    if (id.x >= push.dstWidth || id.y >= push.dstHeight) return;

    float2 uv = (id.xy + 0.5) / float2(push.dstWidth, push.dstHeight);
    float depth = srcTex.SampleLevel(uv, 0); // 1 tap = min из 2×2

    uint2 srcSize;
    srcTex.GetDimensions(srcSize.x, srcSize.y);
    uint2 base = id.xy * 2;

    if (srcSize.x & 1) {
        float e = srcTex.Load(int3(base + uint2(2, 0), 0));
        float f = srcTex.Load(int3(base + uint2(2, 1), 0));
        depth = min(depth, min(e, f));
    }
    if (srcSize.y & 1) {
        float g = srcTex.Load(int3(base + uint2(0, 2), 0));
        float h = srcTex.Load(int3(base + uint2(1, 2), 0));
        depth = min(depth, min(g, h));
    }
    if ((srcSize.x & 1) && (srcSize.y & 1)) {
        depth = min(depth, srcTex.Load(int3(base + uint2(2, 2), 0)));
    }

    dstMip[id.xy] = depth;
}
