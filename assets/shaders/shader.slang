// === SET 0 ===
struct CameraData
{
    float4x4 cameraSpaceMatrix;
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float4x4 invViewProj;
    float4   cameraPositionAndPadding;
    float4   frustumPlanes[6];
};
[[vk::binding(0, 0)]] StructuredBuffer<CameraData> camera;

struct SunData
{
    float4x4 lightSpaceMatrix;
    float4   sunDirection;
    float4   sunColor;
    float4   sunAmbient;
    float4   shadowMapSize; // x: width, y: height, z: 1.0/width, w: 1.0/height
};
[[vk::binding(1, 0)]] StructuredBuffer<SunData> sun;

// === SET 1 ===
struct ModelData
{
    float4 AABBMin;
    float4 AABBMax;
    uint   transformIndex;
    uint   materialIndex;
    uint   drawCommandIndex;
};
[[vk::binding(0, 1)]] StructuredBuffer<ModelData>     objectBuffer;

struct TransformData
{
    float4x4 model;
};
[[vk::binding(1, 1)]] StructuredBuffer<TransformData> transformBuffer;

[[vk::binding(3, 1)]] StructuredBuffer<uint>          visibleIndicesBuffer;

// === SET 2 ===
[[vk::binding(0, 2)]] Sampler2D        textureArray[2048];
[[vk::binding(1, 2)]] Sampler2DShadow  shadowMap;

struct MaterialData
{
    uint  textureIndex;
    uint  normalMapIndex;
    uint  metallicRoughnessIndex;
    uint  emissiveIndex;
    float alphaCutoff;
    uint  alphaMode;
    float emissiveStrength;
    float padding0;
    
    float3 emissiveFactor;
    float  padding1;
};
[[vk::binding(2, 2)]] StructuredBuffer<MaterialData> materialBuffer;

// === CONST ===
static const float PI            = 3.14159265359f;
static const float MIN_ROUGHNESS = 0.005f;

// 0 == opaque pass, 1 == alpha-test pass
[[vk::constant_id(0)]] const int ALPHA_TEST_ENABLED = 0;

// === Vertex I/O ===
struct VSInput
{
    float3 inPosition;
    float3 inColor;
    float3 inNormal;
    float2 inTexCoord;
    float4 inTangent;
};

struct VSOutput
{
    float4 pos            : SV_Position;
    float3 fragColor;
    float3 fragNormal;
    float3 fragTangent;
    float3 fragBitangent;
    float2 fragTexCoord;
    float4 shadowCoord;
    float3 fragWorldPos;
    float  viewDepth;

    nointerpolation uint textureIndex;
    nointerpolation uint normalMapIndex;
    nointerpolation uint metallicRoughnessIndex;
    nointerpolation uint emissiveIndex;
    nointerpolation float alphaCutoff;
    nointerpolation uint  alphaMode;
    nointerpolation float emissiveStrength;
    nointerpolation float3 emissiveFactor;
};

[shader("vertex")]
VSOutput vertMain(VSInput input, uint instanceID : SV_InstanceID, uint baseInstance : SV_StartInstanceLocation)
{
    uint       objectIndex = visibleIndicesBuffer[baseInstance + instanceID];
    ModelData  model       = objectBuffer[objectIndex];
    float4x4   modelMatrix = transformBuffer[model.transformIndex].model;

    float4 worldPos = mul(modelMatrix, float4(input.inPosition, 1.0));

    // Normal matrix 
    float3x3 normalMatrix = (float3x3)modelMatrix;

    float3 N = normalize(mul(normalMatrix, input.inNormal));
    float3 T = normalize(mul(normalMatrix, input.inTangent.xyz));
    T = normalize(T - dot(T, N) * N);           // Gram-Schmidt
    float3 B = cross(N, T) * input.inTangent.w;

    MaterialData mat = materialBuffer[model.materialIndex];

    VSOutput output;
    output.pos                    = mul(camera[0].cameraSpaceMatrix, worldPos);
    output.viewDepth              = output.pos.w;
    output.shadowCoord            = mul(sun[0].lightSpaceMatrix, worldPos);
    output.fragColor              = input.inColor;
    output.fragTexCoord           = input.inTexCoord;
    output.fragNormal             = N;
    output.fragTangent            = T;
    output.fragBitangent          = B;
    output.fragWorldPos           = worldPos.xyz;

    output.textureIndex           = mat.textureIndex;
    output.normalMapIndex         = mat.normalMapIndex;
    output.metallicRoughnessIndex = mat.metallicRoughnessIndex;
    output.emissiveIndex          = mat.emissiveIndex;
    output.alphaCutoff            = mat.alphaCutoff;
    output.alphaMode              = mat.alphaMode;
    output.emissiveStrength       = mat.emissiveStrength;
    output.emissiveFactor         = mat.emissiveFactor;

    return output;
}

// Helper func
float3 SampleNormalMap(uint mapIndex, float2 uv, float2 dx, float2 dy)
{
    float3 raw = textureArray[NonUniformResourceIndex(mapIndex)].SampleGrad(uv, dx, dy).rgb;
    return normalize(raw * 2.0 - 1.0);
}

float ComputeShadow(float3 projCoords, float2 texelSize, float3 surfaceNormal, float3 lightDir)
{
    // Early-out
    if (any(projCoords.xy < 0.0) || any(projCoords.xy > 1.0) ||
        projCoords.z <= 0.0 || projCoords.z >= 1.0)
        return 1.0;

    float bias = max(0.0005 * (1.0 - dot(surfaceNormal, lightDir)), 0.00005);

    float shadow = 0.0;
    [unroll]
    for (int x = -1; x <= 1; ++x)
    {
        [unroll]
        for (int y = -1; y <= 1; ++y)
        {
            float2 offset = float2(x, y) * texelSize;
            shadow += shadowMap.SampleCmpLevelZero(projCoords.xy + offset, projCoords.z + bias);
        }
    }

    return shadow / 9.0;
}

// Cook-Torrance BRDF
float D_GGX(float NdotH, float a)
{
    float a2 = a * a;
    float d  = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

// Correlated Smith GGX Visibility Term (combines G and the denominator)
float V_SmithGGXCorrelated(float NdotV, float NdotL, float a)
{
    float ggxV = NdotL * (NdotV * (1.0 - a) + a);
    float ggxL = NdotV * (NdotL * (1.0 - a) + a);
    return 0.5 / (ggxV + ggxL + 1e-5);
}

float3 F_Schlick(float cosTheta, float3 F0)
{
    // Fast Spherical Gaussian approximation
    return F0 + (1.0 - F0) * exp2((-5.55473 * cosTheta - 6.98316) * cosTheta);
}

float3 ApplyFog(float3 color, float viewDepth)
{
    const float  fogStart = 2000.0;
    const float  fogEnd   = 15000.0;
    const float3 fogColor = float3(0.0, 0.7, 0.8);

    float factor = saturate((viewDepth - fogStart) / (fogEnd - fogStart));
    return lerp(color, fogColor, factor);
}

struct FragOutput
{
    float4 color    : SV_Target0;
    float4 vsNormal : SV_Target1;
};

[shader("fragment")]
FragOutput fragMain(VSOutput input)
{
    float2 dx = ddx(input.fragTexCoord);
    float2 dy = ddy(input.fragTexCoord);

    // === MATERIAL SAMPLING ===
    float4 albedo = textureArray[NonUniformResourceIndex(input.textureIndex)].SampleGrad(input.fragTexCoord, dx, dy);

    // Only in alpha test pipeline 
    if (ALPHA_TEST_ENABLED == 1)
    {
        if (input.alphaMode == 1 && albedo.a < input.alphaCutoff)
            discard;
    }

    float3 emissive = textureArray[NonUniformResourceIndex(input.emissiveIndex)].SampleGrad(input.fragTexCoord, dx, dy).rgb;
    emissive *= input.emissiveFactor * input.emissiveStrength;

    float4 mrSample  = textureArray[NonUniformResourceIndex(input.metallicRoughnessIndex)].SampleGrad(input.fragTexCoord, dx, dy);
    float  roughness = max(mrSample.g, MIN_ROUGHNESS);
    float  metallic  = saturate(mrSample.b);

    // === NORMAL MAPPING ===
    float3 N = normalize(input.fragNormal);
    float3 T = normalize(input.fragTangent);
    float3 B = normalize(input.fragBitangent);

    float3 mapNormal   = SampleNormalMap(input.normalMapIndex, input.fragTexCoord, dx, dy);
    float3 finalNormal = normalize(mapNormal.x * T + mapNormal.y * B + mapNormal.z * N);

    // === LIGHTING SETUP ===
    float3 L = normalize(sun[0].sunDirection.xyz);
    float3 V = normalize(camera[0].cameraPositionAndPadding.xyz - input.fragWorldPos);
    float3 H = normalize(L + V);

    float NdotL = max(dot(finalNormal, L), 0.0);
    float NdotV = max(dot(finalNormal, V), 1e-4);
    float NdotH = max(dot(finalNormal, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    // === SHADOW (PCF 3x3) ===
    float2 shadowTexelSize = sun[0].shadowMapSize.zw;

    float3 projCoords = input.shadowCoord.xyz / input.shadowCoord.w;
    projCoords.xy     = projCoords.xy * 0.5 + 0.5;
    float  shadow     = ComputeShadow(projCoords, shadowTexelSize, finalNormal, L);

    // === PBR (Cook-Torrance) ===
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo.rgb, metallic);

    float  a        = roughness * roughness;
    float  D        = D_GGX(NdotH, a);
    float  Vis      = V_SmithGGXCorrelated(NdotV, NdotL, a);
    float3 F        = F_Schlick(HdotV, F0);
    float3 specular = D * Vis * F;

    float3 kD      = (1.0 - F) * (1.0 - metallic);
    float3 diffuse = kD * albedo.rgb / PI;

    float3 sunRadiance = sun[0].sunColor.rgb * sun[0].sunColor.a;
    float3 ambient     = sun[0].sunAmbient.rgb * sun[0].sunAmbient.a * albedo.rgb;

    float3 litColor = (diffuse + specular) * sunRadiance * NdotL * shadow 
                      + ambient 
                      + emissive;

    litColor *= input.fragColor;

    // === FINAL OUTPUT ===
    float outAlpha = (input.alphaMode == 2) ? albedo.a : 1.0;

    float3 viewNormal = mul((float3x3)camera[0].viewMatrix, finalNormal);

    FragOutput output;
    output.color    = float4(ApplyFog(litColor, input.viewDepth), outAlpha);
    output.vsNormal = float4(normalize(viewNormal), 1.0);
    return output;
}