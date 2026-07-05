// === SET 0 ===
struct CameraData
{
	float4x4 cameraSpaceMatrix;
	float4x4 viewMatrix;
	float4x4 projMatrix;
	float4x4 invViewProj;
	float4 cameraPositionAndPadding;
	float4 frustumPlanes[6];
};
[[vk::binding(0, 0)]]
StructuredBuffer<CameraData> camera;

struct DirectionalLightData
{
	float4x4 lightSpaceMatrix;
	float4 direction;
	float4 color;
	float4 ambient;
	float4 shadowMapSize; // x: width, y: height, z: 1.0/width, w: 1.0/height
	float4 frustumPlanes[6];
	float shadowCasterRange;
	float3 _pad;
	float4 cameraFrustumLightSpaceBounds;
};
[[vk::binding(1, 0)]]
StructuredBuffer<DirectionalLightData> directionalLight;

struct PointLightData
{
	float3 position;
	float radius;
	float3 color;
	float intensity;
	float3 direction; // for spot = normalized direction, for point ignored
	float innerConeAngle; // for point = -1.0
	float outerConeAngle; // for point = -1.0
	uint type;            // 0 = point, 1 = spot
	float2 _pad;
};

[[vk::binding(2, 0)]]
StructuredBuffer<PointLightData> lights;

[[vk::binding(3, 0)]]
StructuredBuffer<uint> lightCount;

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
[[vk::binding(4, 0)]]
StructuredBuffer<SHProbeEntry> shProbes;

[[vk::binding(5, 0)]]
StructuredBuffer<uint> shProbeCount;

// === SET 1 ===
struct ModelData
{
	float4 AABBMin;
	float4 AABBMax;
	uint transformIndex;
	uint materialIndex;
	uint drawCommandIndex;
};
[[vk::binding(0, 1)]]
StructuredBuffer<ModelData> objectBuffer;

struct TransformData
{
	float4x4 model;
};
[[vk::binding(1, 1)]]
StructuredBuffer<TransformData> transformBuffer;

[[vk::binding(3, 1)]]
StructuredBuffer<uint> visibleIndicesBuffer;

// === SET 2 ===
[[vk::binding(0, 2)]]
Sampler2D textureArray[2048];
[[vk::binding(1, 2)]]
Sampler2DShadow shadowMap;

struct MaterialData
{
	uint textureIndex;
	uint normalMapIndex;
	uint metallicRoughnessIndex;
	uint emissiveIndex;
	float alphaCutoff;
	uint alphaMode;
	float emissiveStrength;
	uint doubleSided;
	float4 baseColorFactor;  // offset 32 — matches alignas(16) glm::vec4 in C++ struct
	float3 emissiveFactor;
	float emissivePadding;
	float roughnessFactor;
	float metallicFactor;
	float padding1;
	float padding2;
};
[[vk::binding(2, 2)]]
StructuredBuffer<MaterialData> materialBuffer;

// === CONST ===
static const float PI = 3.14159265359f;
static const float MIN_ROUGHNESS = 0.04f;
static const float MAX_REFLECTION_LOD = 4.0f;

[[vk::constant_id(0)]]
const int ALPHA_TEST_ENABLED = 0;

[[vk::constant_id(1)]]
const int SKYBOX_ENABLED = 1;

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
    float4 pos : SV_Position;
    float3 fragColor;
    float3 fragNormal;
    float3 fragTangent;
    float3 fragBitangent;
    float2 fragTexCoord;
    float4 shadowCoord;
    float3 fragWorldPos;

	nointerpolation uint materialIndex;
};

[shader("vertex")]
VSOutput vertMain(VSInput input, uint instanceID: SV_InstanceID, uint baseInstance: SV_StartInstanceLocation)
{
	uint objectIndex = visibleIndicesBuffer[baseInstance + instanceID];
	ModelData model = objectBuffer[objectIndex];
    float4x4 modelMatrix = transformBuffer[model.transformIndex].model;
    float3x3 modelMatrix3x3 = (float3x3)modelMatrix;

	float4 worldPos = mul(modelMatrix, float4(input.inPosition, 1.0));

	float3x3 normalMatrix = float3x3(
		cross(modelMatrix3x3[1], modelMatrix3x3[2]),
		cross(modelMatrix3x3[2], modelMatrix3x3[0]),
		cross(modelMatrix3x3[0], modelMatrix3x3[1]));
	float3 worldNormal = normalize(mul(normalMatrix, input.inNormal));

	// Tangents transform with the model matrix itself, not the cofactor.
	float3 worldTangent = normalize(mul(modelMatrix3x3, input.inTangent.xyz));
	float3 orthoTangent = worldTangent - dot(worldTangent, worldNormal) * worldNormal;
	float orthoTangentLength = length(orthoTangent);
	float3 worldBitangent;
	if (orthoTangentLength > 1e-6)
	{
		worldTangent = orthoTangent / orthoTangentLength;
		worldBitangent = cross(worldNormal, worldTangent) * input.inTangent.w;
	}
	else
	{
		float3 fallbackUp = abs(worldNormal.y) < 0.9 ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
		worldTangent = normalize(cross(worldNormal, fallbackUp));
		worldBitangent = cross(worldNormal, worldTangent);
	}

	VSOutput output;
	output.pos = mul(camera[0].cameraSpaceMatrix, worldPos);
	output.shadowCoord = mul(directionalLight[0].lightSpaceMatrix, worldPos);
	output.fragColor = input.inColor;
	output.fragTexCoord = input.inTexCoord;
	output.fragNormal = worldNormal;
	output.fragTangent = worldTangent;
	output.fragBitangent = worldBitangent;
	output.fragWorldPos = worldPos.xyz;
	output.materialIndex = model.materialIndex;

	return output;
}

float ComputeShadow(float4 shadowCoord, float2 texelSize, float3 surfaceNormal, float3 lightDir)
{
    float3 projCoords = shadowCoord.xyz / shadowCoord.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    if (any(projCoords.xy < 0.0) || any(projCoords.xy > 1.0) || projCoords.z <= 0.0 || projCoords.z >= 1.0) return 1.0;

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

float3 SampleNormalMap(uint mapIndex, float2 uv, float2 dx, float2 dy)
{
    float3 raw = textureArray[NonUniformResourceIndex(mapIndex)].SampleGrad(uv, dx, dy).rgb;
    return normalize(raw * 2.0 - 1.0);
}

struct FragOutput
{
	float4 color    : SV_Target0;
};

[shader("fragment")]
FragOutput fragMain(VSOutput input, bool isFrontFace: SV_IsFrontFace)
{
	MaterialData mat = materialBuffer[input.materialIndex];

	float2 dx = ddx(input.fragTexCoord);
	float2 dy = ddy(input.fragTexCoord);

	// === MATERIAL SAMPLING ===
	float4 albedo = textureArray[NonUniformResourceIndex(mat.textureIndex)].SampleGrad(input.fragTexCoord, dx, dy)
	              * mat.baseColorFactor * float4(input.fragColor.xyz, 1.0);

	if (ALPHA_TEST_ENABLED == 1)
	{
		if (mat.alphaMode == 1 && albedo.a < mat.alphaCutoff) discard;
	}

	float3 emissive = textureArray[NonUniformResourceIndex(mat.emissiveIndex)].SampleGrad(input.fragTexCoord, dx, dy).rgb
	                * mat.emissiveFactor * mat.emissiveStrength;

    float3 N = normalize(input.fragNormal);
    if (!isFrontFace) N = -N;
    float3 T = normalize(input.fragTangent);
    float3 B = normalize(input.fragBitangent);

    float3 mapNormal = SampleNormalMap(mat.normalMapIndex, input.fragTexCoord, dx, dy);
    float3 finalNormal = normalize(mapNormal.x * T + mapNormal.y * B + mapNormal.z * N);
    float3 L = normalize(directionalLight[0].direction.xyz);

    float NdotL = max(dot(finalNormal, L), 0.0);
    float shadow = ComputeShadow(input.shadowCoord, directionalLight[0].shadowMapSize.zw, finalNormal, L);
    float3 directionalLightRadiance = directionalLight[0].color.rgb * directionalLight[0].color.a;
    float3 constantAmbient = directionalLight[0].ambient.rgb * directionalLight[0].ambient.a;

    float3 radiance = albedo.rgb / PI * NdotL * shadow * directionalLightRadiance + constantAmbient + emissive;

    FragOutput output;
    output.color = float4(radiance, 1.0);
	return output;
}