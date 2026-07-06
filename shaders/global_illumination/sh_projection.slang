static const float F_PI = 3.14159265358979f;
static const uint THREAD_COUNT = 256;

struct SHProbeEntry
{
    float3 position;     float influenceRadius;
    float3 sh0; float _p0;
    float3 sh1; float _p1;
    float3 sh2; float _p2;
    float3 sh3; float _p3;
    float3 sh4; float _p4;
    float3 sh5; float _p5;
    float3 sh6; float _p6;
    float3 sh7; float _p7;
    float3 sh8; float _p8;
};

// Set 0 = globalSet
[[vk::binding(4, 0)]] RWStructuredBuffer<SHProbeEntry> probeArray;

// Set 1 = textureSet
[[vk::binding(5, 1)]] SamplerCube cubemap;

[[vk::push_constant]]
cbuffer PushConstants
{
    int cubemapResolution;
    int probeSlot;
};

float3 faceDirection(int face, float u, float v)
{
    float3 dir;
    switch (face)
    {
    case 0: dir = float3(1.0f, -v, -u); break;
    case 1: dir = float3(-1.0f, -v, u); break;
    case 2: dir = float3(u, 1.0f, v); break;
    case 3: dir = float3(u, -1.0f, -v); break;
    case 4: dir = float3(u, -v, 1.0f); break;
    case 5: dir = float3(-u, -v, -1.0f); break;
    default: dir = float3(0.0f, 0.0f, 1.0f); break;
    }
    return normalize(dir);
}

float[9] computeShBasis(float3 dir)
{
    float outBasis[9];
    const float pi = 3.14159265359f;

    // Normalization coefficients
    const float f0 = 0.5f * sqrt(1.0f / pi);
    const float f1 = sqrt(3.0f / (4.0f * pi));
    const float f2 = 0.5f * sqrt(15.0f / pi);
    const float f3 = 0.25f * sqrt(5.0f / pi);
    const float f4 = 0.25f * sqrt(15.0f / pi);

    outBasis[0] = f0;
    outBasis[1] = -f1 * dir.y;
    outBasis[2] = f1 * dir.z;
    outBasis[3] = -f1 * dir.x;
    outBasis[4] = f2 * dir.x * dir.y;
    outBasis[5] = -f2 * dir.y * dir.z;
    outBasis[6] = f3 * (3.0f * dir.z * dir.z - 1.0f);
    outBasis[7] = -f2 * dir.x * dir.z;
    outBasis[8] = f4 * (dir.x * dir.x - dir.y * dir.y);
    return outBasis;
}

// Ramamoorthi cosine-lobe convolution: A0=pi, A1=2pi/3, A2=pi/4
static const float cosineLobe[9] = {
    F_PI,
    2.0f * F_PI / 3.0f, 2.0f * F_PI / 3.0f, 2.0f * F_PI / 3.0f,
    F_PI * 0.25f, F_PI * 0.25f, F_PI * 0.25f, F_PI * 0.25f, F_PI * 0.25f,
};

void writeCoeff(int band, float3 value)
{
    switch (band)
    {
    case 0: probeArray[probeSlot].sh0 = value; break;
    case 1: probeArray[probeSlot].sh1 = value; break;
    case 2: probeArray[probeSlot].sh2 = value; break;
    case 3: probeArray[probeSlot].sh3 = value; break;
    case 4: probeArray[probeSlot].sh4 = value; break;
    case 5: probeArray[probeSlot].sh5 = value; break;
    case 6: probeArray[probeSlot].sh6 = value; break;
    case 7: probeArray[probeSlot].sh7 = value; break;
    case 8: probeArray[probeSlot].sh8 = value; break;
    }
}

groupshared float3 gsCoeff[THREAD_COUNT];
groupshared float gsWeight[THREAD_COUNT];

[shader("compute")]
[numthreads(THREAD_COUNT, 1, 1)]
void computeMain(uint3 groupThreadID : SV_GroupThreadID)
{
    uint tid = groupThreadID.x;

    float3 coeff[9];
    for (int i = 0; i < 9; ++i) coeff[i] = float3(0.0f, 0.0f, 0.0f);
    float weight = 0.0f;
    float backfaceWeight = 0.0f;

    uint faceTexels = uint(cubemapResolution * cubemapResolution);
    for (uint t = tid; t < 6u * faceTexels; t += THREAD_COUNT)
    {
        int face = int(t / faceTexels);
        uint local = t % faceTexels;
        int x = int(local % uint(cubemapResolution));
        int y = int(local / uint(cubemapResolution));

        float u = (2.0f * (x + 0.5f) / cubemapResolution) - 1.0f;
        float v = (2.0f * (y + 0.5f) / cubemapResolution) - 1.0f;
        float3 direction = faceDirection(face, u, v);
        float4 texel = cubemap.SampleLevel(direction, 0);

        // Approximation of the differential solid angle (pixel weight)
        float w = 1.0f / pow(1.0f + u * u + v * v, 1.5f);

        float basis[9] = computeShBasis(direction);
        for (int i = 0; i < 9; ++i)
            coeff[i] += texel.rgb * basis[i] * w;
        weight += w;
        if (texel.a < 0.5f) backfaceWeight += w;
    }

    gsWeight[tid] = weight;
    GroupMemoryBarrierWithGroupSync();
    for (uint s = THREAD_COUNT / 2; s > 0; s >>= 1)
    {
        if (tid < s) gsWeight[tid] += gsWeight[tid + s];
        GroupMemoryBarrierWithGroupSync();
    }
    float totalWeight = gsWeight[0];
    float normFactor = (4.0f * F_PI) / totalWeight;

    GroupMemoryBarrierWithGroupSync();
    gsWeight[tid] = backfaceWeight;
    GroupMemoryBarrierWithGroupSync();
    for (uint s = THREAD_COUNT / 2; s > 0; s >>= 1)
    {
        if (tid < s) gsWeight[tid] += gsWeight[tid + s];
        GroupMemoryBarrierWithGroupSync();
    }
    if (tid == 0) probeArray[probeSlot]._p0 = gsWeight[0] / totalWeight;

    for (int band = 0; band < 9; ++band)
    {
        GroupMemoryBarrierWithGroupSync();
        gsCoeff[tid] = coeff[band];
        GroupMemoryBarrierWithGroupSync();
        for (uint s = THREAD_COUNT / 2; s > 0; s >>= 1)
        {
            if (tid < s) gsCoeff[tid] += gsCoeff[tid + s];
            GroupMemoryBarrierWithGroupSync();
        }
        if (tid == 0) writeCoeff(band, gsCoeff[0] * normFactor * cosineLobe[band]);
    }
}
