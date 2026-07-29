#include "Shared/GpuStructs.h"
#include "Shared/Bindings.h"

import Common.GI;
import Common.Constants;
import Common.Shadow;
import Common.SH;

// === SET 0 ===
[[vk::binding(BIND_GLOBAL_CAMERA, 0)]]
StructuredBuffer<CameraData> camera;

[[vk::binding(BIND_GLOBAL_SUN, 0)]]
StructuredBuffer<DirectionalLightData> directionalLight;

[[vk::binding(BIND_GLOBAL_POINT_LIGHTS, 0)]]
StructuredBuffer<PointLightData> lights;

[[vk::binding(BIND_GLOBAL_POINT_LIGHT_COUNT, 0)]]
StructuredBuffer<uint> lightCount;

[[vk::binding(BIND_GLOBAL_SH_PROBES, 0)]]
StructuredBuffer<SHProbeEntry> shProbes;

[[vk::binding(BIND_GLOBAL_SH_GRID_INFO, 0)]]
StructuredBuffer<SHGridInfo> shGridInfo;

// === SET 1 ===
[[vk::binding(BIND_MODEL_PRIMITIVES, 1)]]
StructuredBuffer<ModelData> objectBuffer;

[[vk::binding(BIND_MODEL_TRANSFORMS, 1)]]
StructuredBuffer<TransformData> transformBuffer;

[[vk::binding(BIND_MODEL_VISIBLE_INDICES, 1)]]
StructuredBuffer<uint> visibleIndicesBuffer;

// === SET 2 ===
[[vk::binding(BIND_TEXTURES_ARRAY, 2)]]
Sampler2D textureArray[MAX_BINDLESS_TEXTURES];
[[vk::binding(BIND_TEXTURES_SHADOW_MAP, 2)]]
Sampler2DShadow shadowMap;

[[vk::binding(BIND_TEXTURES_MATERIALS, 2)]]
StructuredBuffer<MaterialData> materialBuffer;

// === CONST ===

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
	output.pos = bakeFaceClip(worldPos.xyz, bakePush.probePos, bakePush.faceIdx, shGridInfo[0].captureRange);
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

float3 SampleNormalMap(uint mapIndex, float2 uv, float2 dx, float2 dy)
{
    float3 raw = textureArray[NonUniformResourceIndex(mapIndex)].SampleGrad(uv, dx, dy).rgb;
    return normalize(raw * 2.0 - 1.0);
}

float3 getProbeCoeff(uint idx, uint band)
{
    switch (band)
    {
    case 0: return shProbes[idx].sh0;
    case 1: return shProbes[idx].sh1;
    case 2: return shProbes[idx].sh2;
    default: return shProbes[idx].sh3;
    }
}

float3 sampleSHProbes(float3 worldPos, float3 N)
{
    SHGridInfo grid = shGridInfo[0];

    float3 sh[4];

    if (grid.probeCount <= 1)
    {
        for (int k = 0; k < 4; ++k) sh[k] = getProbeCoeff(0, k);
        return computeIrradiance(N, sh);
    }

    float3 biasedPos = worldPos + N * (grid.spacing * PROBE_NORMAL_BIAS);
    float3 local = (biasedPos - grid.origin) / grid.spacing;
    float3 clamped = clamp(local, float3(0.0, 0.0, 0.0), float3(grid.count - int3(1, 1, 1)));
    float skyW = saturate(length(local - clamped));

    int3 base = min(int3(clamped), max(grid.count - int3(2, 2, 2), int3(0, 0, 0)));
    float3 f = clamped - float3(base);

    for (int k = 0; k < 4; ++k) sh[k] = float3(0.0, 0.0, 0.0);

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
        for (int k = 0; k < 4; ++k)
            sh[k] += getProbeCoeff(slot, k) * w;
        totalW += w;
    }

    if (totalW < 1e-4f)
    {
        for (int k = 0; k < 4; ++k) sh[k] = getProbeCoeff(0, k);
        return computeIrradiance(N, sh);
    }

    for (int k = 0; k < 4; ++k) sh[k] /= totalW;

    if (skyW > 0.0)
    {
        for (int k = 0; k < 4; ++k)
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
    float shadow = ComputeShadow(shadowMap, input.shadowCoord, directionalLight[0].shadowMapSize.zw, finalNormal, L);
    float3 directionalLightRadiance = directionalLight[0].color.rgb * directionalLight[0].color.a;

    float3 radiance = albedo.rgb / PI * NdotL * shadow * directionalLightRadiance
                + albedo.rgb / PI * sampleSHProbes(input.fragWorldPos, finalNormal) * shGridInfo[0].giBounceMultiplier
                + shGridInfo[0].giAmbient + emissive;

    FragOutput output;
    output.color = float4(radiance, 1.0);
	return output;
}