[[vk::binding(0, 0)]] Sampler2D textureArray[];
[[vk::binding(4, 0)]] RWTexture2DArray<float4> outCubemap;

struct PushConstants {
    uint hdrTextureIndex;
};

[[vk::push_constant]]
PushConstants pushConstants;

// Compute shader to convert equirectangular HDR to cubemap
[shader("compute")]
[numthreads(8, 8, 1)]
void computeMain(uint3 threadId : SV_DispatchThreadID)
{
    uint width, height, elements;
    outCubemap.GetDimensions(width, height, elements);

    if (threadId.x >= width || threadId.y >= height)
        return;

    float2 uv = float2(threadId.xy) / float2(width, height);
    // mapped to [-1, 1]
    uv = uv * 2.0 - 1.0;

    // Cubemap face directions
    float3 dir = float3(0, 0, 0);

    // X is right/left (face 0, 1)
    // Y is up/down (face 2, 3) 
    // Z is forward/backward (face 4, 5)
    switch(threadId.z) {
        case 0: dir = float3(1.0, -uv.y, -uv.x); break; // +X
        case 1: dir = float3(-1.0, -uv.y, uv.x); break;  // -X
        case 2: dir = float3(uv.x, 1.0, uv.y); break;   // +Y
        case 3: dir = float3(uv.x, -1.0, -uv.y); break;  // -Y
        case 4: dir = float3(uv.x, -uv.y, 1.0); break;   // +Z
        case 5: dir = float3(-uv.x, -uv.y, -1.0); break; // -Z
    }

    dir = normalize(dir);

    // Convert 3D direction to 2D equirectangular coordinates
    const float2 invAtan = float2(0.1591, 0.3183);
    float2 equirectUV = float2(atan2(dir.z, dir.x), asin(dir.y));
    equirectUV *= invAtan;
    equirectUV += 0.5;

    // Sample the HDR texture
    float4 color = textureArray[pushConstants.hdrTextureIndex].SampleLevel(equirectUV, 0);

    // Write to the cubemap face
    outCubemap[threadId] = color;
}
