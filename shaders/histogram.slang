// === Set 0: MainColor ===
[vk::binding(0, 0)]
Texture2D<float4> sourceTexture;

// === Set 1: exposureSet ===
[vk::binding(0, 1)]
RWStructuredBuffer<uint> histogramBuffer;

[vk::binding(1, 1)]
StructuredBuffer<float> prevExposureBuffer; // we dont use this 

// === Push Constants ===
struct PushConstants {
    float minLogLum;   // log2(minimum visible luminance)
    float logLumRange; // log2(max/min)
    uint2 resolution;
};

[[vk::push_constant]]
PushConstants push;

groupshared uint sharedHistogram[256];

[shader("compute")]
[numthreads(16, 16, 1)]
void computeMain(uint3 id: SV_DispatchThreadID, uint groupIndex: SV_GroupIndex)
{
    sharedHistogram[groupIndex] = 0;
    GroupMemoryBarrierWithGroupSync();

    uint x = id.x;
    uint y = id.y;
    if (x < push.resolution.x && y < push.resolution.y) {
        float4 color = sourceTexture.Load(int3(x, y, 0));
        float luminance = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));
        float logLum = log2(max(luminance, 1e-6));
        int binIndex = int(clamp((logLum - push.minLogLum) / push.logLumRange * 256.0, 0.0, 255.0));

        InterlockedAdd(sharedHistogram[binIndex], 1);
    }

    GroupMemoryBarrierWithGroupSync();

    if (sharedHistogram[groupIndex] > 0) {
        InterlockedAdd(histogramBuffer[groupIndex], sharedHistogram[groupIndex]);
    }
}