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

struct SHGridInfo
{
    float3 origin; float spacing;
    int3 count;    uint probeCount;
    float captureRange; float3 giAmbient;
    float giBounceMultiplier;
};
[[vk::binding(5, 0)]]
StructuredBuffer<SHGridInfo> shGridInfo;

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
static const float PROBE_NORMAL_BIAS = 0.5f;
static const float PROBE_BACKFACE_THRESHOLD = 0.25f;

[[vk::constant_id(0)]]
const int ALPHA_TEST_ENABLED = 0;

[[vk::constant_id(1)]]
const int SKYBOX_ENABLED = 1;

// === Bake face camera ===
struct BakePush
{
	float3 probePos;
	uint faceIdx;
};
[[vk::push_constant]]
BakePush bakePush;

// Must match faceFrustumPlanes() in gi_bake_cull.slang and faceDirection() in sh_projection.slang
static const float3 kFaceForward[6] = {
	float3(1, 0, 0), float3(-1, 0, 0), float3(0, 1, 0), float3(0, -1, 0), float3(0, 0, 1), float3(0, 0, -1),
};
static const float3 kFaceUp[6] = {
	float3(0, 1, 0), float3(0, 1, 0), float3(0, 0, -1), float3(0, 0, 1), float3(0, 1, 0), float3(0, 1, 0),
};
// Reversed-Z: near/far swapped — perspectiveRH_ZO(90, 1, captureRange, 0.01) with Vulkan Y-flip, in analytic form
static const float kBakeFar = 0.01f;

float4 bakeFaceClip(float3 worldPos)
{
	float3 f = kFaceForward[bakePush.faceIdx];
	float3 s = normalize(cross(f, kFaceUp[bakePush.faceIdx]));
	float3 u = cross(s, f);
	float3 rel = worldPos - bakePush.probePos;
	float3 view = float3(dot(s, rel), dot(u, rel), -dot(f, rel));

	// b = -(zFar*zNear)/(zFar-zNear), glm sign convention: positive with swapped reversed-Z near/far
	const float range = shGridInfo[0].captureRange;
	const float a = kBakeFar / (range - kBakeFar);
	const float b = (kBakeFar * range) / (range - kBakeFar);
	return float4(view.x, -view.y, a * view.z + b, -view.z);
}

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
	output.pos = bakeFaceClip(worldPos.xyz);
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

float3 computeIrradiance(float3 n, float3 sh[9])
{
    const float f0 = 0.28209479f;
    const float f1 = 0.48860251f;
    const float f2 = 1.09254843f;
    const float f3 = 0.31539156f;
    const float f4 = 0.54627421f;

    float3 color = sh[0] * f0;
    color += sh[1] * (-f1 * n.y);
    color += sh[2] * (f1 * n.z);
    color += sh[3] * (-f1 * n.x);
    color += sh[4] * (f2 * n.x * n.y);
    color += sh[5] * (-f2 * n.y * n.z);
    color += sh[6] * (f3 * (3.0f * n.z * n.z - 1.0f));
    color += sh[7] * (-f2 * n.x * n.z);
    color += sh[8] * (f4 * (n.x * n.x - n.y * n.y));

    return max(color, float3(0.0));
}

float3 getProbeCoeff(uint idx, uint band)
{
    switch (band)
    {
    case 0: return shProbes[idx].sh0;
    case 1: return shProbes[idx].sh1;
    case 2: return shProbes[idx].sh2;
    case 3: return shProbes[idx].sh3;
    case 4: return shProbes[idx].sh4;
    case 5: return shProbes[idx].sh5;
    case 6: return shProbes[idx].sh6;
    case 7: return shProbes[idx].sh7;
    default: return shProbes[idx].sh8;
    }
}

float3 sampleSHProbes(float3 worldPos, float3 N)
{
    SHGridInfo grid = shGridInfo[0];

    float3 sh[9];

    if (grid.probeCount <= 1)
    {
        for (int k = 0; k < 9; ++k) sh[k] = getProbeCoeff(0, k);
        return computeIrradiance(N, sh);
    }

    float3 biasedPos = worldPos + N * (grid.spacing * PROBE_NORMAL_BIAS);
    float3 local = (biasedPos - grid.origin) / grid.spacing;
    float3 clamped = clamp(local, float3(0.0, 0.0, 0.0), float3(grid.count - int3(1, 1, 1)));
    float skyW = saturate(length(local - clamped));

    int3 base = min(int3(clamped), max(grid.count - int3(2, 2, 2), int3(0, 0, 0)));
    float3 f = clamped - float3(base);

    for (int k = 0; k < 9; ++k) sh[k] = float3(0.0, 0.0, 0.0);

    float totalW = 0.0;

    [unroll]
    for (int c = 0; c < 8; ++c)
    {
        int3 corner = int3(c & 1, (c >> 1) & 1, (c >> 2) & 1);
        int3 idx = min(base + corner, grid.count - int3(1, 1, 1));
        float3 axisW = lerp(float3(1.0, 1.0, 1.0) - f, f, float3(corner));

        // Slot layout matches the bake loop in bakeAll: x fastest, slot 0 = skybox
        uint slot = 1u + uint(idx.x + grid.count.x * (idx.y + grid.count.y * idx.z));

        // Smooth backface validity
        float validity = saturate(1.0 - shProbes[slot]._p0 / PROBE_BACKFACE_THRESHOLD);
        if (validity <= 0.0) continue;

        // Wrap weight kills probes behind the shaded surface, blocking through-wall leaks
        // (Majercik et al. 2019, "Dynamic Diffuse Global Illumination with Ray-Traced Irradiance Fields")
        float3 probePos = grid.origin + grid.spacing * float3(idx);
        float3 dirToProbe = normalize(probePos - worldPos);
        float wrap = (dot(dirToProbe, N) + 1.0) * 0.5;
        float dirW = wrap * wrap + 0.05;

        float w = axisW.x * axisW.y * axisW.z * validity * dirW;
        for (int k = 0; k < 9; ++k)
            sh[k] += getProbeCoeff(slot, k) * w;
        totalW += w;
    }

    if (totalW < 1e-4f)
    {
        for (int k = 0; k < 9; ++k) sh[k] = getProbeCoeff(0, k);
        return computeIrradiance(N, sh);
    }

    for (int k = 0; k < 9; ++k) sh[k] /= totalW;

    if (skyW > 0.0)
    {
        for (int k = 0; k < 9; ++k)
            sh[k] = lerp(sh[k], getProbeCoeff(0, k), skyW);
    }

    return computeIrradiance(N, sh);
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

	// Backface marker (alpha 0): sh_projection turns the marked fraction into probe validity
	if (!isFrontFace && mat.doubleSided != 1)
	{
		FragOutput output;
		output.color = float4(0.0, 0.0, 0.0, 0.0);
		return output;
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

    float3 radiance = albedo.rgb / PI * NdotL * shadow * directionalLightRadiance
                + albedo.rgb / PI * sampleSHProbes(input.fragWorldPos, finalNormal) * shGridInfo[0].giBounceMultiplier
                + shGridInfo[0].giAmbient + emissive;

    FragOutput output;
    output.color = float4(radiance, 1.0);
	return output;
}