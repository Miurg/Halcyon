[vk::binding(0, 0)] Sampler2D depthTexture;
[vk::binding(1, 0)] Sampler2D normalsTexture;
[vk::binding(2, 0)] Sampler2D noiseTexture;

struct CameraData
{
    float4x4 cameraSpaceMatrix;
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float4   cameraPositionAndPadding;
    float4   frustumPlanes[6];
};
[[vk::binding(0, 1)]] StructuredBuffer<CameraData> camera;

struct PushConstants
{
    int   kernelSize;
    float radius;
    float bias;
    float power;
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

// ---- Helpers ----

// Reconstruct view-space position from UV + raw depth buffer value.
float3 reconstructViewPos(float2 uv, float depth)
{
    float4x4 proj = camera[0].projMatrix;
    float viewZ = proj[3][2] / (-depth - proj[2][2]);
    float negViewZ = -viewZ;
    float2 ndc = uv * 2.0 - 1.0;
    float3 viewPos;
    viewPos.x = ndc.x * negViewZ / proj[0][0];
    viewPos.y = ndc.y * negViewZ / proj[1][1];
    viewPos.z = viewZ;
    return viewPos;
}

// Pre-computed 32-sample hemisphere kernel (cosine-weighted, accelerated toward center)
static const float3 ssaoKernel[32] = {
    float3(-0.134, 0.044, 0.018), float3( 0.046,-0.042, 0.037), float3(-0.098, 0.034, 0.076), float3( 0.051,-0.091, 0.050),
    float3( 0.130, 0.043, 0.088), float3(-0.057, 0.149, 0.029), float3( 0.079,-0.105, 0.143), float3(-0.163, 0.025, 0.129),
    float3( 0.120, 0.183, 0.020), float3(-0.043,-0.168, 0.183), float3( 0.214,-0.038, 0.088), float3(-0.101, 0.071, 0.237),
    float3( 0.182, 0.136, 0.145), float3(-0.236,-0.123, 0.068), float3( 0.063, 0.275, 0.113), float3(-0.195, 0.204, 0.090),
    float3( 0.301, 0.047, 0.139), float3(-0.145,-0.254, 0.192), float3( 0.244, 0.208, 0.172), float3(-0.084, 0.327, 0.113),
    float3( 0.129,-0.318, 0.178), float3(-0.353, 0.050, 0.160), float3( 0.297, 0.229, 0.186), float3(-0.214,-0.275, 0.244),
    float3( 0.370, 0.172, 0.106), float3(-0.115, 0.401, 0.044), float3( 0.168,-0.329, 0.271), float3(-0.367,-0.146, 0.225),
    float3( 0.401, 0.132, 0.224), float3(-0.230, 0.370, 0.174), float3( 0.289,-0.280, 0.302), float3(-0.400, 0.219, 0.197),
};

// ---- Main SSAO ----

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target
{
    float depth = depthTexture.SampleLevel(input.uv, 0).r;
    if (depth >= 1.0)
        return float4(1.0, 1.0, 1.0, 1.0);

    float3 fragPos = reconstructViewPos(input.uv, depth);

    // View-space normal from g-buffer
    float3 normal = normalize(normalsTexture.SampleLevel(input.uv, 0).xyz);

    // Read random rotation vector from 4x4 tiled noise texture
    float2 noiseScale = float2(input.pos.xy) / 4.0;
    float3 randomVec = normalize(float3(
        noiseTexture.SampleLevel(noiseScale, 0).xy * 2.0 - 1.0,
        0.0
    ));

    // Build TBN to orient hemisphere along surface normal
    float3 tangent  = normalize(randomVec - normal * dot(randomVec, normal));
    float3 binormal = cross(normal, tangent);
    float3x3 TBN    = float3x3(tangent, binormal, normal);

    float4x4 proj = camera[0].projMatrix;
    float projA = proj[2][2];
    float projB = proj[3][2];
    float occlusion = 0.0;

    int samples = min(pc.kernelSize, 32);
    for (int i = 0; i < samples; i++)
    {
        // Use pre-computed kernel direction, rotated by TBN
        float3 sampleDir = mul(ssaoKernel[i], TBN);
        float3 samplePos = fragPos + sampleDir * pc.radius;

        // Project sample back to screen UV
        float4 sampleClip = mul(proj, float4(samplePos, 1.0));
        float2 sampleNDC = sampleClip.xy / sampleClip.w;
        float2 sampleUV = sampleNDC * 0.5 + 0.5;
        sampleUV = clamp(sampleUV, float2(0.001, 0.001), float2(0.999, 0.999));

        // Read actual depth and linearize (just Z, no full reconstruction)
        float sampleRawDepth = depthTexture.SampleLevel(sampleUV, 0).r;
        if (sampleRawDepth >= 1.0) continue;

        float sampleZ = projB / (-sampleRawDepth - projA);

        // In RH view space: Z is negative, closer = less negative = LARGER Z
        // If actual surface Z > sample Z => surface is closer => sample is occluded
        float rangeCheck = smoothstep(0.0, 1.0, pc.radius / max(abs(fragPos.z - sampleZ), 0.0001));
        occlusion += (sampleZ >= samplePos.z + pc.bias ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion = 1.0 - (occlusion / float(pc.kernelSize));
    occlusion = pow(saturate(occlusion), pc.power);

    return float4(occlusion, occlusion, occlusion, 1.0);
}
