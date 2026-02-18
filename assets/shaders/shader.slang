// === SET 0 ===

struct CameraData
{
    float4x4 cameraSpaceMatrix;
};
[[vk::binding(0, 0)]] StructuredBuffer<CameraData> camera;

struct SunData
{
    float4x4 lightSpaceMatrix;
    float4   sunDirection;
    float4   sunColor;    // .rgb = color, .a = intensity
    float4   sunAmbient;  // .rgb = color, .a = intensity
};
[[vk::binding(1, 0)]] StructuredBuffer<SunData> sun;

// === SET 1 ===

struct ModelData
{
    float4 AABBMin;
    float4 AABBMax;
    uint   transformIndex;
    uint   materialIndex;
    uint   drawCommandIndex; // unused
};
[[vk::binding(0, 1)]] StructuredBuffer<ModelData>    objectBuffer;

struct TransformData
{
    float4x4 model;
};
[[vk::binding(1, 1)]] StructuredBuffer<TransformData> transformBuffer;

[[vk::binding(3, 1)]] StructuredBuffer<uint>          visibleIndicesBuffer;

// === SET 2 ===

[[vk::binding(0, 2)]] Sampler2D         textureArray[2048];
[[vk::binding(1, 2)]] Sampler2DShadow   shadowMap;

struct MaterialData
{
    uint textureIndex;
    uint normalMapIndex;
};
[[vk::binding(2, 2)]] StructuredBuffer<MaterialData> materialBuffer;

// Vertex I/O

struct VSInput
{
    float3 inPosition;
    float3 inColor;
    float3 inNormal;
    float2 inTexCoord;
    float4 inTangent; // .w = bitangent sign
};

struct VSOutput
{
    float4 pos           : SV_Position;
    float3 fragColor;
    float3 fragNormal;
    float3 fragTangent;
    float3 fragBitangent;
    float2 fragTexCoord;
    float4 shadowCoord;
    float  viewDepth;
    nointerpolation uint textureIndex;
    nointerpolation uint normalMapIndex;
};

[shader("vertex")]
VSOutput vertMain(VSInput input, uint instanceID : SV_InstanceID, uint baseInstance : SV_StartInstanceLocation)
{
    uint       objectIndex = visibleIndicesBuffer[baseInstance + instanceID];
    ModelData  model       = objectBuffer[objectIndex];
    float4x4   modelMatrix = transformBuffer[model.transformIndex].model;

    float4 worldPos = mul(modelMatrix, float4(input.inPosition, 1.0));

    // Normal matrix (upper 3x3 is sufficient for uniform scale)
    float3x3 normalMatrix = (float3x3)modelMatrix;

    float3 N = normalize(mul(normalMatrix, input.inNormal));
    float3 T = normalize(mul(normalMatrix, input.inTangent.xyz));
    // Gram-Schmidt re-orthogonalization
    T        = normalize(T - dot(T, N) * N);
    float3 B = cross(N, T) * input.inTangent.w;

    MaterialData mat = materialBuffer[model.materialIndex];

    VSOutput output;
    output.pos           = mul(camera[0].cameraSpaceMatrix, worldPos);
    output.viewDepth     = output.pos.w;
    output.shadowCoord   = mul(sun[0].lightSpaceMatrix, worldPos);
    output.fragColor     = input.inColor;
    output.fragTexCoord  = input.inTexCoord;
    output.fragNormal    = N;
    output.fragTangent   = T;
    output.fragBitangent = B;
    output.textureIndex  = mat.textureIndex;
    output.normalMapIndex = mat.normalMapIndex;
    return output;
}


// === Helper functions ===

float3 SampleNormalMap(uint mapIndex, float2 uv, float2 dx, float2 dy)
{
    float3 raw = textureArray[NonUniformResourceIndex(mapIndex)].SampleGrad(uv, dx, dy).rgb;
    return normalize(raw * 2.0 - 1.0); // remap [0,1] -> [-1,1]
}

float ComputeShadow(float3 projCoords, float3 surfaceNormal, float3 lightDir)
{
    // Check if fragment is inside the shadow frustum
    bool inFrustum = saturate(projCoords.x) == projCoords.x
                  && saturate(projCoords.y) == projCoords.y
                  && projCoords.z > 0.0
                  && projCoords.z < 1.0;

    if (!inFrustum)
        return 1.0;

    uint width, height, levels;
    shadowMap.GetDimensions(0, width, height, levels);
    float2 texelSize = 1.0 / float2(width, height);

    float bias = max(0.0005 * (1.0 - dot(surfaceNormal, lightDir)), 0.00005);

    float shadow = 0.0;
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
            shadow += shadowMap.SampleCmpLevelZero(projCoords.xy + float2(x, y) * texelSize, projCoords.z + bias);

    return shadow / 9.0;
}

float3 ApplyFog(float3 color, float viewDepth)
{
    const float  fogStart = 20.0;
    const float  fogEnd   = 150.0;
    const float3 fogColor = float3(0.0, 0.7, 0.8);

    float factor = saturate((viewDepth - fogStart) / (fogEnd - fogStart));
    return lerp(color, fogColor, factor);
}

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET
{
    float2 dx = ddx(vertIn.fragTexCoord);
    float2 dy = ddy(vertIn.fragTexCoord);

    float4 albedo = textureArray[NonUniformResourceIndex(vertIn.textureIndex)].SampleGrad(vertIn.fragTexCoord, dx, dy);
    if (albedo.a < 0.1)
        discard;

    // Reconstruct world-space normal from normal map via TBN
    float3 N = normalize(vertIn.fragNormal);
    float3 T = normalize(vertIn.fragTangent);
    float3 B = normalize(vertIn.fragBitangent);
    float3 mapNormal   = SampleNormalMap(vertIn.normalMapIndex, vertIn.fragTexCoord, dx, dy);
    float3 finalNormal = normalize(mapNormal.x * T + mapNormal.y * B + mapNormal.z * N);

    // Shadow
    float3 projCoords = vertIn.shadowCoord.xyz / vertIn.shadowCoord.w;
    projCoords.xy     = projCoords.xy * 0.5 + 0.5;
    float3 lightDir   = normalize(sun[0].sunDirection.xyz);
    float  shadow     = ComputeShadow(projCoords, finalNormal, lightDir);

    // Diffuse + ambient lighting
    float  diff     = max(dot(finalNormal, lightDir), 0.0);
    float3 sunLight = sun[0].sunColor.rgb   * sun[0].sunColor.a   * diff * shadow;
    float3 ambient  = sun[0].sunAmbient.rgb * sun[0].sunAmbient.a;
    float3 litColor = (sunLight + ambient) * albedo.rgb * vertIn.fragColor;

    return float4(ApplyFog(litColor, vertIn.viewDepth), albedo.a);
}