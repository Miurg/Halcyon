#include "Shared/GpuStructs.h"
#include "Shared/Bindings.h"

import Common.GI;
import Common.Constants;
import Common.BRDF;
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

[[vk::binding(BIND_GLOBAL_GTAO_TEXTURE, 0)]]
Sampler2D gtaoTexture;

[[vk::binding(BIND_GLOBAL_REFLECTION_PROBES, 0)]]
StructuredBuffer<ReflectionProbeData> reflectionProbes;
[[vk::binding(BIND_GLOBAL_REFLECTION_PROBE_COUNT, 0)]]
StructuredBuffer<uint> reflectionProbeCount;

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

[[vk::binding(BIND_TEXTURES_PREFILTERED_MAP, 2)]]
SamplerCube prefilteredMap;
[[vk::binding(BIND_TEXTURES_BRDF_LUT, 2)]]
Sampler2D brdfLutMap;
[[vk::binding(BIND_TEXTURES_REFLECTION_CUBEMAPS, 2)]]
SamplerCube reflectionProbeMaps[MAX_REFLECTION_PROBES];

// === CONST ===
static const float REFLECTION_BLEND_MARGIN = 1.0f; // world-space width of the probe-to-probe/sky transition

[[vk::constant_id(0)]]
const int ALPHA_TEST_ENABLED = 0;

[[vk::constant_id(1)]]
const int SKYBOX_ENABLED = 1;

[[vk::constant_id(2)]]
const int GTAO_ENABLED = 1;

[[vk::constant_id(3)]]
const int ALPHA_TO_COVERAGE = 0;

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

	float3x3 normalMatrix =
	    float3x3(cross(modelMatrix3x3[1], modelMatrix3x3[2]), cross(modelMatrix3x3[2], modelMatrix3x3[0]),
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
	output.fragColor = input.inColor;
	output.fragTexCoord = input.inTexCoord;
	output.fragNormal = worldNormal;
	output.fragTangent = worldTangent;
	output.fragBitangent = worldBitangent;
	output.fragWorldPos = worldPos.xyz;
	output.materialIndex = model.materialIndex;

	return output;
}

// === Texture sampling helpers ===

// === IBL / Ambient ===

float3 getProbeCoeff(uint idx, uint band)
{
	switch (band)
	{
	case 0:
		return shProbes[idx].sh0;
	case 1:
		return shProbes[idx].sh1;
	case 2:
		return shProbes[idx].sh2;
	default:
		return shProbes[idx].sh3;
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
		if (shProbes[slot]._p0 > PROBE_BACKFACE_THRESHOLD) continue;

		float w = axisW.x * axisW.y * axisW.z;
		for (int k = 0; k < 4; ++k) sh[k] += getProbeCoeff(slot, k) * w;
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
		for (int k = 0; k < 4; ++k) sh[k] = lerp(sh[k], getProbeCoeff(0, k), skyW);
	}

	return computeIrradiance(N, sh);
}

// Box-projected local reflection blended across every covering probe: each probe's weight
// falls off toward its AABB face, contributions are normalized when over-covered, and the
// unfilled remainder is left for the skybox (Lagarde, "Image-based Lighting approaches", 2012).
// Returns coverage in [0,1]; `color` is the weighted probe reflection.
float sampleReflectionProbes(float3 worldPos, float3 R, float roughness, out float3 color)
{
	color = float3(0.0, 0.0, 0.0);
	float totalW = 0.0;
	uint count = min(reflectionProbeCount[0], MAX_REFLECTION_PROBES);

	for (uint i = 0; i < count; ++i)
	{
		ReflectionProbeData p = reflectionProbes[i];
		float3 center = (p.boxMin + p.boxMax) * 0.5;
		float3 halfExtent = (p.boxMax - p.boxMin) * 0.5;

		// Distance from the nearest face along each axis; negative = outside the box.
		float3 toFace = halfExtent - abs(worldPos - center);
		float inside = min(min(toFace.x, toFace.y), toFace.z);
		float w = saturate(inside / REFLECTION_BLEND_MARGIN);
		if (w <= 0.0) continue;

		float3 invR = 1.0 / R;
		float3 tPlan = max((p.boxMax - worldPos) * invR, (p.boxMin - worldPos) * invR);
		float t = min(min(tPlan.x, tPlan.y), tPlan.z);
		float3 dir = (worldPos + R * t) - p.captureOrigin;

		color += reflectionProbeMaps[NonUniformResourceIndex(p.cubemapIndex)]
		             .SampleLevel(dir, roughness * MAX_REFLECTION_LOD)
		             .rgb *
		         w;
		totalW += w;
	}

	// Normalize where probes overlap (blend), keep partial coverage where they don't (sky fills in).
	if (totalW > 1.0)
	{
		color /= totalW;
		totalW = 1.0;
	}
	return totalW;
}

// Computes indirect / ambient lighting: split-sum IBL diffuse from SH probes,
// split-sum IBL specular from prefiltered cubemap + BRDF LUT, plus a constant
// ambient term from the directional light. Falls back to constant-only when
// the skybox specialization constant is disabled.
float3 ComputeAmbient(Surface s, MaterialData mat, float3 worldPos, float4 dirLightAmbient, float ao)
{
	float3 F_ibl = F_SchlickRoughness(s.NdotV, s.F0, s.roughness);
	float3 startAmbient = dirLightAmbient.a * dirLightAmbient.rgb;

	if (SKYBOX_ENABLED == 1)
	{
		float3 irradiance = sampleSHProbes(worldPos, s.N);
		float3 iblDiffuse =
		    (1.0 - F_ibl) * (1.0 - s.metallic) * DiffuseOrenNayarIBL(s.albedo, s.NdotV, s.roughness) * irradiance;

		float3 iblSpecular = float3(0.0, 0.0, 0.0);
		if (mat.alphaMode != 1 && mat.doubleSided != 1)
		{
			float3 R = reflect(-s.V, s.N);
			float3 probeColor;
			float coverage = sampleReflectionProbes(worldPos, R, s.roughness, probeColor);
			float3 prefiltColor = probeColor;
			[branch]
			if (coverage < 1.0)
			{
				float3 skyColor = prefilteredMap.SampleLevel(R, s.roughness * MAX_REFLECTION_LOD).rgb;

				prefiltColor += skyColor * (1.0 - coverage);
			}
			float2 brdfLut = brdfLutMap.SampleLevel(float2(s.NdotV, s.roughness), 0).rg;
			iblSpecular = prefiltColor * (F_ibl * brdfLut.x + brdfLut.y);
		}

		return (iblDiffuse + iblSpecular + startAmbient) * ao;
	}
	else
	{
		// Fallback without skybox: ambient * Oren-Nayar lambertian only.
		return (1.0 - F_ibl) * (1.0 - s.metallic) * DiffuseOrenNayarIBL(s.albedo, s.NdotV, s.roughness) * ao +
		       startAmbient;
	}
}

struct SurfaceContext
{
	MaterialData mat;
	float3 albedo;
	float alpha = 1.0;
	float3 emissive;
	float roughness;
	float metallic;
	float3 normal;
};

SurfaceContext SetupSurface(VSOutput input, bool isFrontFace)
{
	SurfaceContext surface;

	surface.mat = materialBuffer[input.materialIndex];

	if (ALPHA_TEST_ENABLED == 1)
    {
        float4 albedoFull = textureArray[NonUniformResourceIndex(surface.mat.textureIndex)].Sample(input.fragTexCoord) *
                            surface.mat.baseColorFactor * float4(input.fragColor, 1.0);
        surface.albedo = albedoFull.rgb;
        surface.alpha = albedoFull.a;

		if (ALPHA_TO_COVERAGE == 0)
		{
			if (surface.mat.alphaMode == 1)
			{
				if (surface.alpha < surface.mat.alphaCutoff) discard;
				surface.alpha = 1.0;
			}
		}
		else if (surface.mat.alphaMode == 1)
		{
			// Wyman & McGuire, "Anti-Aliased Alpha Test", JCGT 2017.
			surface.alpha = saturate((surface.alpha - surface.mat.alphaCutoff) / max(fwidth(surface.alpha), 1e-5) + 0.5);
			if (surface.alpha <= 0.0) discard; // skip lighting + ROP for fully-transparent texels
        }

        float2 dx = ddx(input.fragTexCoord);
        float2 dy = ddy(input.fragTexCoord);

		surface.emissive =
		    textureArray[NonUniformResourceIndex(surface.mat.emissiveIndex)].SampleGrad(input.fragTexCoord, dx, dy).rgb *
		    surface.mat.emissiveFactor * surface.mat.emissiveStrength;
		float4 mrSample = textureArray[NonUniformResourceIndex(surface.mat.metallicRoughnessIndex)].SampleGrad(
		    input.fragTexCoord, dx, dy);
		surface.roughness = max(mrSample.g * surface.mat.roughnessFactor, MIN_ROUGHNESS);
		surface.metallic = saturate(mrSample.b * surface.mat.metallicFactor);

        float3 N = normalize(input.fragNormal);
        if (!isFrontFace) N = -N;
        float3 T = normalize(input.fragTangent);
        float3 B = normalize(input.fragBitangent);

        float3 raw = textureArray[NonUniformResourceIndex(surface.mat.normalMapIndex)] .SampleGrad(input.fragTexCoord, dx, dy) .rgb;
        float3 mapNormal = raw * 2.0 - 1.0;
        surface.normal = normalize(mapNormal.x * T + mapNormal.y * B + mapNormal.z * N);
	}
	else
    {
        float4 albedoFull = textureArray[NonUniformResourceIndex(surface.mat.textureIndex)].Sample(input.fragTexCoord) *
                            surface.mat.baseColorFactor * float4(input.fragColor, 1.0);
        surface.albedo = albedoFull.rgb;
        surface.alpha = 1.0;

		surface.emissive =
		    textureArray[NonUniformResourceIndex(surface.mat.emissiveIndex)].Sample(input.fragTexCoord).rgb *
		    surface.mat.emissiveFactor * surface.mat.emissiveStrength;
		float4 mrSample =
		    textureArray[NonUniformResourceIndex(surface.mat.metallicRoughnessIndex)].Sample(input.fragTexCoord);
		surface.roughness = max(mrSample.g * surface.mat.roughnessFactor, MIN_ROUGHNESS);
		surface.metallic = saturate(mrSample.b * surface.mat.metallicFactor);

        float3 N = normalize(input.fragNormal);
        if (!isFrontFace) N = -N;
        float3 T = normalize(input.fragTangent);
        float3 B = normalize(input.fragBitangent);

        float3 raw = textureArray[NonUniformResourceIndex(surface.mat.normalMapIndex)].Sample(input.fragTexCoord) .rgb;
        float3 mapNormal = raw * 2.0 - 1.0;
        surface.normal = normalize(mapNormal.x * T + mapNormal.y * B + mapNormal.z * N);
	}

	surface.roughness = GeometricSpecularAA(surface.normal, surface.roughness);

	return surface;
}

float3 SolveLight(SurfaceContext surface, VSOutput input)
{
	// === LIGHTING SETUP ===
	float3 L = normalize(directionalLight[0].direction.xyz);
	float3 V = normalize(camera[0].cameraPositionAndPadding.xyz - input.fragWorldPos);

	float NdotV = max(dot(surface.normal, V), 1e-4);

	float3 F0 = lerp(float3(0.04, 0.04, 0.04), surface.albedo, surface.metallic);

	Surface s = { surface.albedo, F0, surface.metallic, surface.roughness, surface.normal, V, NdotV };

    // === SHADOW (PCF 3x3) ===
    float shadow = ComputeShadow(shadowMap, mul(directionalLight[0].lightSpaceMatrix, float4(input.fragWorldPos,1.0)), directionalLight[0].shadowMapSize.zw, surface.normal, L);

	// === DIRECTIONAL LIGHT ===
	float3 directionalLightRadiance = directionalLight[0].color.rgb * directionalLight[0].color.a;

	LightingVectors lv = ComputeLightingVectors(L, V, surface.normal);
	BRDFResult brdf = EvaluateBRDF(s, lv, L);

	float3 directLight = (brdf.diffuse + brdf.specular) * directionalLightRadiance * lv.NdotL * shadow;

	// === POINT / SPOT LIGHTS ===
	float3 litColorFromLights = float3(0.0, 0.0, 0.0);
	for (uint i = 0; i < lightCount[0]; i++)
	{
		PointLightData light = lights[i];
		litColorFromLights +=
		    EvaluatePunctualLight(s, input.fragWorldPos, light.position, light.radius, light.color, light.intensity,
		                          light.direction, light.innerConeAngle, light.outerConeAngle, light.type);
	}

	float ao = 1.0;
	if (GTAO_ENABLED == 1 && surface.mat.alphaMode != 2)
    {
        float2 screenUV = input.pos.xy / camera[0].screenSize;
        ao = gtaoTexture.SampleLevel(screenUV, 0).r;
	}

	// === IBL / Ambient ===
	float3 ambient = ComputeAmbient(s, surface.mat, input.fragWorldPos, directionalLight[0].ambient, ao);

	float3 litColor = directLight + litColorFromLights + ambient + surface.emissive;
	return litColor;
}

// === Fragment entry ===
struct FragOutput
{
	float4 color : SV_Target0;
};

[shader("fragment")]
FragOutput fragMain(VSOutput input, bool isFrontFace: SV_IsFrontFace)
{
	SurfaceContext surface = SetupSurface(input, isFrontFace);
	float4 finalColor = float4(SolveLight(surface, input), surface.alpha);

	FragOutput output;
	output.color = finalColor;
	return output;
}
