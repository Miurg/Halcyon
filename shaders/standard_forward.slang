#include "Shared/GpuStructs.h"
#include "Shared/Bindings.h"

import Common.GI;
import Common.Constants;
import Common.BRDF;
import Common.Shadow;
import Common.SH;
import Common.SharedStructs;

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

[[vk::binding(BIND_GLOBAL_FORWARD_CLUSTERED_GRID, 0)]]
StructuredBuffer<ForwardCluster> forwardClusteredGrid;

[[vk::binding(BIND_GLOBAL_FORWARD_CLUSTERED_INFO, 0)]]
StructuredBuffer<uint> forwardClusteredInfo;

[[vk::binding(BIND_GLOBAL_VISIBLE_POINT_LIGHTS, 0)]]
StructuredBuffer<uint> visiblePointLightIndices;

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

float4 PackTBNToQuaternion(float3 T, float3 N, float bitangentSign)
{
	N = normalize(N);
	T = normalize(T - N * dot(N, T));
	float3 B = cross(N, T);

	float3x3 m = float3x3(T.x, B.x, N.x, T.y, B.y, N.y, T.z, B.z, N.z);

	float4 q;
	float trace = m[0][0] + m[1][1] + m[2][2];

	if (trace > 0.0)
	{
		float s = 0.5 / sqrt(trace + 1.0);

		q.w = 0.25 / s;
		q.x = (m[2][1] - m[1][2]) * s;
		q.y = (m[0][2] - m[2][0]) * s;
		q.z = (m[1][0] - m[0][1]) * s;
	}
	else if (m[0][0] > m[1][1] && m[0][0] > m[2][2])
	{
		float s = 2.0 * sqrt(1.0 + m[0][0] - m[1][1] - m[2][2]);

		q.w = (m[2][1] - m[1][2]) / s;
		q.x = 0.25 * s;
		q.y = (m[0][1] + m[1][0]) / s;
		q.z = (m[0][2] + m[2][0]) / s;
	}
	else if (m[1][1] > m[2][2])
	{
		float s = 2.0 * sqrt(1.0 + m[1][1] - m[0][0] - m[2][2]);

		q.w = (m[0][2] - m[2][0]) / s;
		q.x = (m[0][1] + m[1][0]) / s;
		q.y = 0.25 * s;
		q.z = (m[1][2] + m[2][1]) / s;
	}
	else
	{
		float s = 2.0 * sqrt(1.0 + m[2][2] - m[0][0] - m[1][1]);

		q.w = (m[1][0] - m[0][1]) / s;
		q.x = (m[0][2] + m[2][0]) / s;
		q.y = (m[1][2] + m[2][1]) / s;
		q.z = 0.25 * s;
	}

	q = normalize(q);
	return q;
}

struct VSOutput
{
	float4 pos : SV_Position;
	float3 fragColor;
	float4 fragTBNQuaternion;
	float bitangentSign;
	float2 fragTexCoord;

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

	if (orthoTangentLength > 1e-6)
	{
		worldTangent = orthoTangent / orthoTangentLength;
	}
	else
	{
		float3 fallbackUp = abs(worldNormal.y) < 0.9 ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
		worldTangent = normalize(cross(worldNormal, fallbackUp));
	}

	VSOutput output;
	output.pos = mul(camera[0].cameraSpaceMatrix, worldPos);
	output.fragColor = input.inColor;
	output.fragTexCoord = input.inTexCoord;
	output.fragTBNQuaternion = PackTBNToQuaternion(worldTangent, worldNormal, input.inTangent.w);
	output.bitangentSign = input.inTangent.w;
	output.materialIndex = model.materialIndex;

	return output;
}

// === Texture sampling helpers ===

// === IBL / Ambient ===

float3 sampleSHProbes(float3 worldPos, float3 N)
{
	SHGridInfo grid = shGridInfo[0];

	var skyProbe = shProbes[0];

	if (grid.probeCount <= 1)
	{
		float3 skySh[4] = { skyProbe.sh0, skyProbe.sh1, skyProbe.sh2, skyProbe.sh3 };
		return computeIrradiance(N, skySh);
	}

	float3 biasedPos = worldPos + N * (grid.spacing * PROBE_NORMAL_BIAS);

	float3 local = (biasedPos - grid.origin) / grid.spacing;

	float3 gridMax = float3(grid.count - 1);
	float3 clamped = clamp(local, 0.0, gridMax);
	float skyW = saturate(length(local - clamped));

	int3 base = min(int3(clamped), max(grid.count - 2, 0));
	float3 f = clamped - float3(base);

	float3 w0 = 1.0 - f;
	float3 w1 = f;

	float weights[8] = { w0.x * w0.y * w0.z, w1.x * w0.y * w0.z, w0.x * w1.y * w0.z, w1.x * w1.y * w0.z,
		                  w0.x * w0.y * w1.z, w1.x * w0.y * w1.z, w0.x * w1.y * w1.z, w1.x * w1.y * w1.z };

	uint sy = grid.count.x;
	uint sz = grid.count.x * grid.count.y;

	uint baseSlot = 1u + uint(base.x) + uint(base.y) * sy + uint(base.z) * sz;

	uint offsets[8] = { 0, 1, sy, sy + 1, sz, sz + 1, sy + sz, sy + sz + 1 };

	float3 sh0 = 0.0, sh1 = 0.0, sh2 = 0.0, sh3 = 0.0;
	float totalW = 0.0;

	[unroll]
	for (int i = 0; i < 8; ++i)
	{
		uint slot = baseSlot + offsets[i];
		var probe = shProbes[slot];

		float valid = probe._p0 > PROBE_BACKFACE_THRESHOLD ? 0.0 : 1.0;
		float actualW = weights[i] * valid;

		sh0 += probe.sh0 * actualW;
		sh1 += probe.sh1 * actualW;
		sh2 += probe.sh2 * actualW;
		sh3 += probe.sh3 * actualW;
		totalW += actualW;
	}

	float invW = totalW > 0.00001 ? (1.0 / totalW) : 0.0;
	sh0 *= invW;
	sh1 *= invW;
	sh2 *= invW;
	sh3 *= invW;

	if (skyW > 0.0)
	{
		sh0 = lerp(sh0, skyProbe.sh0, skyW);
		sh1 = lerp(sh1, skyProbe.sh1, skyW);
		sh2 = lerp(sh2, skyProbe.sh2, skyW);
		sh3 = lerp(sh3, skyProbe.sh3, skyW);
	}

	float3 finalSH[4] = { sh0, sh1, sh2, sh3 };
	return computeIrradiance(N, finalSH);
}

// Box-projected local reflection blended across every covering probe: each probe's weight
// falls off toward its AABB face, contributions are normalized when over-covered, and the
// unfilled remainder is left for the skybox (Lagarde, "Image-based Lighting approaches", 2012).
// Returns coverage in [0,1]; `color` is the weighted probe reflection.
float sampleReflectionProbes(float3 worldPos, float3 R, float roughness, out float3 color, ForwardCluster cluster)
{
	color = float3(0.0, 0.0, 0.0);
	float totalW = 0.0;
	bool useClusterFallback =
		cluster.offset == INVALID_FORWARD_CLUSTER_OFFSET ||
		(forwardClusteredInfo[1] & FORWARD_CLUSTER_GLOBAL_OVERFLOW_BIT) != 0u;
	uint count = cluster.countReflectProbes;

	if (useClusterFallback)
	{
		uint probeBufferCapacity;
		uint probeBufferStride;
		reflectionProbes.GetDimensions(probeBufferCapacity, probeBufferStride);
		count = min(reflectionProbeCount[0], min(probeBufferCapacity, MAX_REFLECTION_PROBES));
	}

	for (uint i = 0; i < count; ++i)
	{
		uint probeIndex = i;
		[branch]
		if (!useClusterFallback)
			probeIndex = forwardClusteredInfo[cluster.offset + cluster.countLights + i];

		ReflectionProbeData p = reflectionProbes[probeIndex];
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
float3 ComputeAmbient(SurfaceContext surface, GpuMaterialData mat, float3 worldPos, float4 dirLightAmbient, float ao, ForwardCluster cluster)
{
	float3 F_ibl = F_SchlickRoughness(surface.NdotV, surface.F0, surface.roughness);
	float3 startAmbient = dirLightAmbient.a * dirLightAmbient.rgb;

	if (SKYBOX_ENABLED == 1)
	{
		float3 irradiance = sampleSHProbes(worldPos, surface.normal);
		float3 iblDiffuse = (1.0 - F_ibl) * (1.0 - surface.metallic) *
		                    DiffuseOrenNayarIBL(surface.albedo, surface.NdotV, surface.roughness) * irradiance;

		float3 iblSpecular = float3(0.0, 0.0, 0.0);
		if (mat.alphaMode != 1 && mat.doubleSided != 1)
		{
			float3 R = reflect(-surface.viewVector, surface.normal);
            float3 probeColor;
            float coverage = sampleReflectionProbes(worldPos, R, surface.roughness, probeColor, cluster);
			float3 prefiltColor = probeColor;
			[branch]
			if (coverage < 1.0)
			{
				float3 skyColor = prefilteredMap.SampleLevel(R, surface.roughness * MAX_REFLECTION_LOD).rgb;

				prefiltColor += skyColor * (1.0 - coverage);
			}
			float2 brdfLut = brdfLutMap.SampleLevel(float2(surface.NdotV, surface.roughness), 0).rg;
			iblSpecular = prefiltColor * (F_ibl * brdfLut.x + brdfLut.y);
		}

		return (iblDiffuse + iblSpecular + startAmbient) * ao;
	}
	else
	{
		// Fallback without skybox: ambient * Oren-Nayar lambertian only.
		return (1.0 - F_ibl) * (1.0 - surface.metallic) *
		           DiffuseOrenNayarIBL(surface.albedo, surface.NdotV, surface.roughness) * ao +
		       startAmbient;
	}
}

void UnpackQuaternionToTBN(float4 q, out float3 T, out float3 N)
{
	float bitangentSign = q.w < 0.0 ? -1.0 : 1.0;

	q = normalize(q);

	float x2 = q.x + q.x;
	float y2 = q.y + q.y;
	float z2 = q.z + q.z;

	float xx = q.x * x2;
	float xy = q.x * y2;
	float xz = q.x * z2;

	float yy = q.y * y2;
	float yz = q.y * z2;
	float zz = q.z * z2;

	float wx = q.w * x2;
	float wy = q.w * y2;
	float wz = q.w * z2;

	T = float3(1.0 - (yy + zz), xy + wz, xz - wy);

	N = float3(xz + wy, yz - wx, 1.0 - (xx + yy));
}

float3 UnpackQuaternionToNormal(float4 q)
{
	q = normalize(q);

	float x2 = q.x + q.x;
	float y2 = q.y + q.y;
	float z2 = q.z + q.z;

	float xx = q.x * x2;
	float yy = q.y * y2;
	float xz = q.x * z2;
	float yz = q.y * z2;
	float wx = q.w * x2;
	float wy = q.w * y2;

	return float3(xz + wy, yz - wx, 1.0 - (xx + yy));
}

SurfaceContext SetupSurface(VSOutput input, bool isFrontFace)
{
	SurfaceContext surface;

	MaterialData mat = materialBuffer[input.materialIndex];

	float3 rawNormal;
	float4 rawMetalRough;
	float3 rawEmission;
	float4 rawAlbedo;

	if (ALPHA_TEST_ENABLED == 1)
	{
		rawAlbedo = mat.baseColorFactor * float4(input.fragColor, 1.0);
		if (mat.materialFlags & MaterialFlags.HasBaseColorTexture)
		{
			rawAlbedo *= textureArray[NonUniformResourceIndex(mat.textureIndex)].Sample(input.fragTexCoord);
		}

		float2 dx = ddx((float2)input.fragTexCoord);
		float2 dy = ddy((float2)input.fragTexCoord);

		if (ALPHA_TO_COVERAGE == 0)
		{
			if (mat.alphaMode == 1)
			{
				if (rawAlbedo.a < mat.alphaCutoff) discard;
			}
		}
		else if (mat.alphaMode == 1)
		{
			// Wyman & McGuire, "Anti-Aliased Alpha Test", JCGT 2017.
			rawAlbedo.a = saturate((rawAlbedo.a - mat.alphaCutoff) / max(fwidth(rawAlbedo.a), 1e-5) + 0.5);
			if (rawAlbedo.a <= 0.0) discard; // skip lighting + ROP for fully-transparent texels
		}

		rawNormal = textureArray[NonUniformResourceIndex(mat.normalMapIndex)].SampleGrad(input.fragTexCoord, dx, dy).rgb;
		rawMetalRough =
		    textureArray[NonUniformResourceIndex(mat.metallicRoughnessIndex)].SampleGrad(input.fragTexCoord, dx, dy);

		surface.emissive = mat.emissiveFactor * mat.emissiveStrength;
		bool hasEmissiveContribution = any(surface.emissive > float3(0.0));
		if (hasEmissiveContribution && (mat.materialFlags & MaterialFlags.HasEmissiveTexture))
		{
			rawEmission =
			    textureArray[NonUniformResourceIndex(mat.emissiveIndex)].SampleGrad(input.fragTexCoord, dx, dy).rgb;
		}
	}
	else
	{
		rawAlbedo = textureArray[NonUniformResourceIndex(mat.textureIndex)].Sample(input.fragTexCoord);
		rawNormal = textureArray[NonUniformResourceIndex(mat.normalMapIndex)].Sample(input.fragTexCoord).rgb;
		rawMetalRough = textureArray[NonUniformResourceIndex(mat.metallicRoughnessIndex)].Sample(input.fragTexCoord);

		surface.emissive = mat.emissiveFactor * mat.emissiveStrength;
		bool hasEmissiveContribution = any(surface.emissive > float3(0.0));
		if (hasEmissiveContribution && (mat.materialFlags & MaterialFlags.HasEmissiveTexture))
		{
			rawEmission = textureArray[NonUniformResourceIndex(mat.emissiveIndex)].Sample(input.fragTexCoord).rgb;
		}
	}

	GpuMaterialData gpuMat;
	gpuMat.alphaCutoff = mat.alphaCutoff;
	gpuMat.alphaMode = mat.alphaMode;
	gpuMat.doubleSided = mat.doubleSided;
	surface.mat = gpuMat;

	float2 ndcXY = (input.pos.xy / camera[0].screenSize) * 2.0 - 1.0;
	float4 clipPos = float4(ndcXY, input.pos.z, 1.0);
	float4 worldPosUnscaled = mul(camera[0].invViewProj, clipPos);
	float3 worldPosition = worldPosUnscaled.xyz / worldPosUnscaled.w;

	surface.viewVector = normalize(camera[0].cameraPositionAndPadding.xyz - worldPosition);

	float3 N;
	float3 T;
	float3 B;
	UnpackQuaternionToTBN(input.fragTBNQuaternion, T, N);
	if (!isFrontFace) N = -N;
	B = cross(N, T) * input.bitangentSign;
	N = normalize(N);
	T = normalize(T);
	float3 mapNormal = rawNormal * 2.0 - 1.0;
	surface.normal = normalize(mapNormal.x * T + mapNormal.y * B + mapNormal.z * N);
	surface.NdotV = max(dot(surface.normal, surface.viewVector), 1e-4);

	surface.roughness = mat.roughnessFactor;
	surface.metallic = mat.metallicFactor;
	surface.roughness *= rawMetalRough.g;
	surface.metallic *= rawMetalRough.b;
	surface.roughness = max(surface.roughness, MIN_ROUGHNESS);
	surface.metallic = saturate(surface.metallic);
	surface.roughness = GeometricSpecularAA(surface.normal, surface.roughness);

	if (ALPHA_TEST_ENABLED == 1)
	{
		surface.albedo = rawAlbedo.rgb;
		surface.alpha = rawAlbedo.a;
	}
	else
	{
		float4 albedoFull = mat.baseColorFactor * float4(input.fragColor, 1.0) * rawAlbedo;
		surface.albedo = albedoFull.rgb;
		surface.alpha = albedoFull.a;
	}

	surface.F0 = lerp(float3(0.04, 0.04, 0.04), surface.albedo, surface.metallic);

	surface.emissive *= rawEmission;

	return surface;
}

float3 SolveLight(SurfaceContext surface, VSOutput input)
{

    float ao = 1.0;
    if (GTAO_ENABLED == 1 && surface.mat.alphaMode != 2)
    {
        float2 screenUV = input.pos.xy / camera[0].screenSize;
        ao = gtaoTexture.SampleLevel(screenUV, 0).r;
    }

	// === LIGHTING SETUP ===
	float3 lightVector = normalize(directionalLight[0].direction.xyz);

	float2 ndcXY = (input.pos.xy / camera[0].screenSize) * 2.0 - 1.0;
	float4 clipPos = float4(ndcXY, input.pos.z, 1.0);
	float4 worldPosUnscaled = mul(camera[0].invViewProj, clipPos);
	float3 worldPosition = worldPosUnscaled.xyz / worldPosUnscaled.w;

    // === SHADOW (PCF 3x3) ===
    float NdotL = dot(surface.normal, lightVector);
    float shadow = 1.0;
	if (NdotL > 0.0)
        shadow = ComputeShadow(shadowMap, mul(directionalLight[0].lightSpaceMatrix, float4(worldPosition, 1.0)),
                               directionalLight[0].shadowMapSize.zw, surface.normal, lightVector);

	// === DIRECTIONAL LIGHT ===
	float3 directionalLightRadiance = directionalLight[0].color.rgb * directionalLight[0].color.a;

    LightingVectors lv = ComputeLightingVectors(lightVector, surface.viewVector, surface.normal);
    BRDFResult brdf = EvaluateBRDF(surface, lv, lightVector);

	float3 directLight = (brdf.diffuse + brdf.specular) * directionalLightRadiance * lv.NdotL * shadow;

	uint2 tileCount = (uint2(camera[0].screenSize) + TILE_SIZE - 1u) / TILE_SIZE;
	uint2 tile = min(uint2(input.pos.xy) / TILE_SIZE, tileCount - 1u);

	float p22 = camera[0].projMatrix[2][2];
	float p32 = camera[0].projMatrix[3][2];

	float zNear = abs(p32 / (1.0f + p22));
	float zFar = abs(p32 / p22);
	float linearZ = abs(p32 / (-input.pos.z - p22));

	float sliceF = log(max(linearZ, zNear) / zNear) / log(zFar / zNear);
	uint slice = min(uint(sliceF * float(Z_SLICES)), Z_SLICES - 1u);

    uint clusterIndex = tile.x + tile.y * tileCount.x + slice * tileCount.x * tileCount.y;
    ForwardCluster cluster = forwardClusteredGrid[clusterIndex];

	// === POINT / SPOT LIGHTS ===
	float3 litColorFromLights = float3(0.0, 0.0, 0.0);
	bool useClusterFallback =
		cluster.offset == INVALID_FORWARD_CLUSTER_OFFSET ||
		(forwardClusteredInfo[1] & FORWARD_CLUSTER_GLOBAL_OVERFLOW_BIT) != 0u;
	uint currentLightCount = cluster.countLights;

	if (useClusterFallback)
	{
		uint lightBufferCapacity;
		uint lightBufferStride;
		lights.GetDimensions(
			lightBufferCapacity,
			lightBufferStride);

		uint visibleLightBufferCapacity;
		uint visibleLightBufferStride;
		visiblePointLightIndices.GetDimensions(
			visibleLightBufferCapacity,
			visibleLightBufferStride);

		currentLightCount = 0u;
		if (visibleLightBufferCapacity > 0u)
		{
			currentLightCount = min(
				visiblePointLightIndices[0],
				min(
					visibleLightBufferCapacity - 1u,
					min(lightBufferCapacity, MAX_POINT_LIGHTS)));
		}
	}

	for (uint i = 0; i < currentLightCount; i++)
	{
		uint lightIndex;
		[branch]
		if (useClusterFallback)
		{
			lightIndex =
				visiblePointLightIndices[1u + i];
		}
		else
		{
			lightIndex =
				forwardClusteredInfo[cluster.offset + i];
		}

		PointLightData light = lights[lightIndex];
		litColorFromLights +=
		    EvaluatePunctualLight(surface, worldPosition, light.position, light.radius, light.color, light.intensity,
		                          light.direction, light.innerConeAngle, light.outerConeAngle, light.type);
	}

    // === IBL / Ambient ===
    float3 ambient = ComputeAmbient(surface, surface.mat, worldPosition, directionalLight[0].ambient, ao, cluster);

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
