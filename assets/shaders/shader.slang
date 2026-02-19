// === SET 0 ===

struct CameraData
{
    float4x4 cameraSpaceMatrix;
    float3   cameraPosition;
};
[[vk::binding(0, 0)]] StructuredBuffer<CameraData> camera;

struct SunData
{
    float4x4 lightSpaceMatrix;
    float4   sunDirection;
    float4   sunColor;
    float4   sunAmbient;
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
    uint textureIndex;
    uint normalMapIndex;
    uint metallicRoughnessIndex;
    uint emissiveIndex;
};
[[vk::binding(2, 2)]] StructuredBuffer<MaterialData> materialBuffer;

// Vertex I/O

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
};

[shader("vertex")]
VSOutput vertMain(VSInput input, uint instanceID : SV_InstanceID, uint baseInstance : SV_StartInstanceLocation)
{
    uint       objectIndex = visibleIndicesBuffer[baseInstance + instanceID];
    ModelData  model       = objectBuffer[objectIndex];
    float4x4   modelMatrix = transformBuffer[model.transformIndex].model;

    float4 worldPos = mul(modelMatrix, float4(input.inPosition, 1.0));

    float3x3 normalMatrix = (float3x3)modelMatrix;

    float3 N = normalize(mul(normalMatrix, input.inNormal));
    float3 T = normalize(mul(normalMatrix, input.inTangent.xyz));
    T        = normalize(T - dot(T, N) * N);
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
    return output;
}

// === Helper functions ===

float3 SampleNormalMap(uint mapIndex, float2 uv, float2 dx, float2 dy)
{
    float3 raw = textureArray[NonUniformResourceIndex(mapIndex)].SampleGrad(uv, dx, dy).rgb;
    return normalize(raw * 2.0 - 1.0);
}

float ComputeShadow(float3 projCoords, float3 surfaceNormal, float3 lightDir)
{
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
    const float  fogStart = 2000.0;
    const float  fogEnd   = 15000.0;
    const float3 fogColor = float3(0.0, 0.7, 0.8);

    float factor = saturate((viewDepth - fogStart) / (fogEnd - fogStart));
    return lerp(color, fogColor, factor);
}

// === Cook-Torrance BRDF helpers ===

// GGX normal distribution function 
float D_GGX(float NdotH, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float d  = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (3.14159265 * d * d);
}

// Smith GGX geometry occlusion
float G_SmithGGX(float NdotV, float NdotL, float roughness)
{
    float r  = roughness + 1.0;
    float k  = (r * r) / 8.0;
    float gV = NdotV / (NdotV * (1.0 - k) + k);
    float gL = NdotL / (NdotL * (1.0 - k) + k);
    return gV * gL;
}

// Fresnel-Schlick
float3 F_Schlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}


[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET
{
    float2 dx = ddx(vertIn.fragTexCoord);
    float2 dy = ddy(vertIn.fragTexCoord);

    float4 albedo = textureArray[NonUniformResourceIndex(vertIn.textureIndex)].SampleGrad(vertIn.fragTexCoord, dx, dy);
    if (albedo.a < 0.1)
        discard;

    float3 emissive = textureArray[NonUniformResourceIndex(vertIn.emissiveIndex)].SampleGrad(vertIn.fragTexCoord, dx, dy).rgb;
    // Metallic-roughness: G=roughness, B=metallic (glTF packed)
    float4 mrSample  = textureArray[NonUniformResourceIndex(vertIn.metallicRoughnessIndex)].SampleGrad(vertIn.fragTexCoord, dx, dy);
    float  roughness = mrSample.g;
    float  metallic  = mrSample.b;

    // TBN normal
    float3 N = normalize(vertIn.fragNormal);
    float3 T = normalize(vertIn.fragTangent);
    float3 B = normalize(vertIn.fragBitangent);
    float3 mapNormal   = SampleNormalMap(vertIn.normalMapIndex, vertIn.fragTexCoord, dx, dy);
    float3 finalNormal = normalize(mapNormal.x * T + mapNormal.y * B + mapNormal.z * N);

    float3 L = normalize(sun[0].sunDirection.xyz);
    float3 V = normalize(camera[0].cameraPosition - vertIn.fragWorldPos);
    float3 H = normalize(L + V);

    float NdotL = max(dot(finalNormal, L), 0.0);
    float NdotV = max(dot(finalNormal, V), 0.0001);
    float NdotH = max(dot(finalNormal, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    // Shadow
    float3 projCoords = vertIn.shadowCoord.xyz / vertIn.shadowCoord.w;
    projCoords.xy     = projCoords.xy * 0.5 + 0.5;
    float  shadow     = ComputeShadow(projCoords, finalNormal, L);

    // F0: dielectric = 0.04, metall = albedo color
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo.rgb, metallic);

    // Cook-Torrance specular
    float  D   = D_GGX(NdotH, roughness);
    float  G   = G_SmithGGX(NdotV, NdotL, roughness);
    float3 F   = F_Schlick(HdotV, F0);
    float3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 0.0001);

    // Diffuse part: metal does not give diffuse light
    float3 kD       = (1.0 - F) * (1.0 - metallic);
    float3 diffuse  = kD * albedo.rgb / 3.14159265;

    float3 sunRadiance = sun[0].sunColor.rgb * sun[0].sunColor.a;
    float3 ambient     = sun[0].sunAmbient.rgb * sun[0].sunAmbient.a * albedo.rgb;

    float3 litColor = (diffuse + specular) * sunRadiance * NdotL * shadow + ambient + emissive;
    litColor *= vertIn.fragColor;

    return float4(ApplyFog(litColor, vertIn.viewDepth), albedo.a);
}