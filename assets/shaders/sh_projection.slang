static const float F_PI = 3.14159265358979f;

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
[[vk::binding(3, 1)]] SamplerCube cubemap;

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
    case 0: dir = float3(1.0f, v, -u); break;
    case 1: dir = float3(-1.0f, v, u); break;
    case 2: dir = float3(u, 1.0f, -v); break;
    case 3: dir = float3(u, -1.0f, v); break;
    case 4: dir = float3(u, v, 1.0f); break;
    case 5: dir = float3(-u, v, -1.0f); break;
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

// Local-only struct for accumulation during projection
struct SHCoefficients
{
    float3 coefficient[9];
};

SHCoefficients convolveWithCosineLobe(SHCoefficients SHIn)
{
    SHCoefficients sh = SHIn;
    // L0
    sh.coefficient[0].x *= 1.0f;
    sh.coefficient[0].y *= 1.0f;
    sh.coefficient[0].z *= 1.0f;

    // L1
    const float f1 = 2.0f / 3.0f;
    for (int i = 1; i <= 3; ++i)
    {
        sh.coefficient[i].x *= f1;
        sh.coefficient[i].y *= f1;
        sh.coefficient[i].z *= f1;
    }

    // L2
    const float f2 = 0.25f;
    for (int i = 4; i <= 8; ++i)
    {
        sh.coefficient[i].x *= f2;
        sh.coefficient[i].y *= f2;
        sh.coefficient[i].z *= f2;
    }
    return sh;
}

[shader("compute")]
[numthreads(1, 1, 1)]
void computeMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    SHCoefficients result;
    for (int i = 0; i < 9; i++)
        result.coefficient[i] = float3(0.0f, 0.0f, 0.0f);

    float totalWeight = 0.0f;

    for (int face = 0; face < 6; face++)
    {
        for (int y = 0; y < cubemapResolution; y++)
        {
            for (int x = 0; x < cubemapResolution; x++)
            {
                float u = (2.0f * (x + 0.5f) / cubemapResolution) - 1.0f;
                float v = (2.0f * (y + 0.5f) / cubemapResolution) - 1.0f;
                float3 direction = faceDirection(face, u, v);
                float3 color = cubemap.SampleLevel(direction, 0).rgb;

                // Approximation of the differential solid angle (pixel weight)
                float weight = 1.0f / pow(1.0f + u * u + v * v, 1.5f);

                float basis[9] = computeShBasis(direction);

                for (int i = 0; i < 9; ++i)
                {
                    result.coefficient[i].x += color.x * basis[i] * weight;
                    result.coefficient[i].y += color.y * basis[i] * weight;
                    result.coefficient[i].z += color.z * basis[i] * weight;
                }
                totalWeight += weight;
            }
        }
    }
    float normFactor = (4.0f * F_PI) / totalWeight;

    for (int i = 0; i < 9; ++i)
    {
        result.coefficient[i].x *= normFactor;
        result.coefficient[i].y *= normFactor;
        result.coefficient[i].z *= normFactor;
    }
    result = convolveWithCosineLobe(result);

    probeArray[probeSlot].sh0 = result.coefficient[0];
    probeArray[probeSlot].sh1 = result.coefficient[1];
    probeArray[probeSlot].sh2 = result.coefficient[2];
    probeArray[probeSlot].sh3 = result.coefficient[3];
    probeArray[probeSlot].sh4 = result.coefficient[4];
    probeArray[probeSlot].sh5 = result.coefficient[5];
    probeArray[probeSlot].sh6 = result.coefficient[6];
    probeArray[probeSlot].sh7 = result.coefficient[7];
    probeArray[probeSlot].sh8 = result.coefficient[8];
}
