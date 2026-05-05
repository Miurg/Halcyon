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

[[vk::binding(6, 2)]]
SamplerCube prefilteredMap;
[[vk::binding(7, 2)]]
Sampler2D brdfLutMap;

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
	float viewDepth;

	nointerpolation uint materialIndex;
};

[shader("vertex")]
VSOutput vertMain(VSInput input, uint instanceID: SV_InstanceID, uint baseInstance: SV_StartInstanceLocation)
{
	uint objectIndex = visibleIndicesBuffer[baseInstance + instanceID];
	ModelData model = objectBuffer[objectIndex];
	float4x4 modelMatrix = transformBuffer[model.transformIndex].model;

	float4 worldPos = mul(modelMatrix, float4(input.inPosition, 1.0));
	float3x3 M = (float3x3)modelMatrix;
	float3x3 normalMatrix = float3x3(
		cross(M[1], M[2]),
		cross(M[2], M[0]),
		cross(M[0], M[1]));

	float3 N = normalize(mul(normalMatrix, input.inNormal));
	// Tangents transform with the model matrix itself, not the cofactor.
	float3 T = normalize(mul(M, input.inTangent.xyz));
	float3 tOrtho = T - dot(T, N) * N;
	float tLen = length(tOrtho);
	float3 B;
	if (tLen > 1e-6)
	{
		T = tOrtho / tLen;
		B = cross(N, T) * input.inTangent.w;
	}
	else
	{
		float3 up = abs(N.y) < 0.9 ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
		T = normalize(cross(N, up));
		B = cross(N, T);
	}

	VSOutput output;
	output.pos = mul(camera[0].cameraSpaceMatrix, worldPos);
	output.viewDepth = output.pos.w;
	output.shadowCoord = mul(directionalLight[0].lightSpaceMatrix, worldPos);
	output.fragColor = input.inColor;
	output.fragTexCoord = input.inTexCoord;
	output.fragNormal = N;
	output.fragTangent = T;
	output.fragBitangent = B;
	output.fragWorldPos = worldPos.xyz;
	output.materialIndex = model.materialIndex;

	return output;
}

// Samples a normal map and unpacks from [0,1] to [-1,1] tangent-space range.
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

// Computes PCF shadow factor with a 3x3 kernel using hardware comparison sampler.
// Applies normal-based bias to reduce shadow acne on surfaces at grazing angles.
// Returns 1.0 (fully lit) for fragments outside the shadow map bounds.
float ComputeShadow(float3 projCoords, float2 texelSize, float3 surfaceNormal, float3 lightDir)
{
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

// GGX normal distribution function (Trowbridge & Reitz, 1975 / Walter et al., 2007).
// Describes the statistical distribution of microfacet orientations on a surface.
// Preferred over Beckmann due to longer specular tail matching real-world materials.
//        a^2
// ──────────────────────────
// PI * ((NdotH^2 * (a^2 - 1) + 1)^2)
float D_GGX(float NdotH, float a)
{
	float a2 = a * a;
	float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
	return a2 / (PI * d * d);
}

// Correlated Smith visibility function for GGX distribution (Heitz, 2014).
// Models mutual masking-shadowing between microfacets more accurately
// than the uncorrelated separable form used in earlier formulations.
//                    0.5
// ──────────────────────────────────────────────
// NdotL*(NdotV*(1-a)+a) + NdotV*(NdotL*(1-a)+a)
float V_SmithGGXCorrelated(float NdotV, float NdotL, float a)
{
	float ggxV = NdotL * (NdotV * (1.0 - a) + a);
	float ggxL = NdotV * (NdotL * (1.0 - a) + a);
	return 0.5 / (ggxV + ggxL + 1e-5);
}

// Schlick approximation of the Fresnel term (Schlick, 1994).
// Approximates the fraction of light reflected vs refracted at a surface.
// Uses fast exp2 form instead of pow for performance.
// F0 + (1 - F0) * 2^((-5.55473 * HdotV - 6.98316) * HdotV)
float3 F_Schlick(float HdotV, float3 F0)
{
    return F0 + (1.0 - F0) * exp2((-5.55473 * HdotV - 6.98316) * HdotV);
}

// Simplest diffuse BRDF — Lambertian reflectance normalized by PI.
float3 DiffuseLambert(float3 albedo)
{
	return albedo / PI;
}

// Diffuse BRDF that models rough surface scattering using microfacet theory.
// Based on "Generalization of Lambert's Reflectance Model" (Oren & Nayar, 1994).
float3 DiffuseOrenNayar(float3 albedo, float NdotL, float NdotV, float roughness,
                        float3 L, float3 V, float3 N)
{
	float sigma2 = roughness * roughness;
	float A = 1.0 - 0.5 * sigma2 / (sigma2 + 0.57);
	float B = 0.45 * sigma2 / (sigma2 + 0.09);

	float3 Lperp = L - N * NdotL;
	float3 Vperp = V - N * NdotV;
	float LperpLen = length(Lperp);
	float VperpLen = length(Vperp);

	float cosPhiDiff = (LperpLen > 1e-4 && VperpLen > 1e-4)
		? saturate(dot(Lperp / LperpLen, Vperp / VperpLen))
		: 0.0;

	float sinThetaI = sqrt(max(1.0 - NdotL * NdotL, 0.0));
	float sinThetaR = sqrt(max(1.0 - NdotV * NdotV, 0.0));
	float s = sinThetaI * sinThetaR / max(max(NdotL, NdotV), 1e-4);

	return albedo / PI * (A + B * cosPhiDiff * s);
}

float3 DiffuseOrenNayarIBL(float3 albedo, float NdotV, float roughness)
{
    float sigma2 = roughness * roughness;
    float A = 1.0 - 0.5 * sigma2 / (sigma2 + 0.57);
    // Without B, we dont have L in ambient
    return albedo / PI * A;
}

// Disney diffuse BRDF with retroreflection and grazing darkening.
// Based on "Physically Based Shading at Disney" (Burley, SIGGRAPH 2012).
float3 DiffuseDisney(float3 albedo, float NdotL, float NdotV, float HdotV, float roughness)
{
	float FL   = pow(1.0 - NdotL, 5.0);
	float FV   = pow(1.0 - NdotV, 5.0);
	float Fd90 = 0.5 + 2.0 * HdotV * HdotV * roughness;
	float Fd   = lerp(1.0, Fd90, FL) * lerp(1.0, Fd90, FV);
	return albedo / PI * Fd;
}

// Increases roughness based on normal map variance to reduce specular aliasing.
// Based on "Stable Specular Highlights" (Tokuyoshi & Kaplanyan, 2019).
// Uses screen-space ddx/ddy of the world normal to estimate high-frequency detail.
float GeometricSpecularAA(float3 worldNormal, float roughness)
{
	float3 dndu = ddx(worldNormal);
	float3 dndv = ddy(worldNormal);

	float variance = 0.5 * (dot(dndu, dndu) + dot(dndv, dndv));

	float roughness2 = roughness * roughness;
	float kernelRoughness2 = min(2.0 * variance, 0.18);
	return sqrt(saturate(roughness2 + kernelRoughness2));
}

// Computes physically-based point light attenuation using inverse-square falloff
// with a smooth windowing function (Karis, UE4) to avoid singularity at dist=0
// and ensure zero contribution at the light radius boundary.
float PointLightAttenuation(float dist, float radius)
{
	float x = saturate(1.0 - pow(dist / radius, 4.0));
	return x * x / (dist * dist + 1.0);
}

struct LightingVectors
{
	float3 H;
	float NdotL;
	float NdotH;
	float HdotV;
};

// Computes per-light half-vector and dot products (NdotL, NdotH, HdotV)
// from a given light direction L, view vector V and surface normal N.
// Call once per light source before EvaluateBRDF.
LightingVectors ComputeLightingVectors(float3 L, float3 V, float3 N)
{
	LightingVectors lv;
	lv.H     = normalize(L + V);
	lv.NdotL = max(dot(N, L), 0.0);
	lv.NdotH = max(dot(N, lv.H), 0.0);
	lv.HdotV = max(dot(lv.H, V), 0.0);
	return lv;
}

struct BRDFResult
{
	float3 diffuse;
	float3 specular;
};

// Evaluates the Cook-Torrance BRDF for a single light source.
// Returns diffuse (Oren-Nayar) and specular (GGX/Smith) components separately.
// NdotV must be precomputed outside the light loop as it is view-invariant.
BRDFResult EvaluateBRDF(float3 albedo, float3 F0, float metallic, float roughness,
                        float NdotL, float NdotV, float NdotH, float HdotV,
                        float3 L, float3 V, float3 N)
{
	float a = roughness * roughness;

	float  D   = D_GGX(NdotH, a);
	float  Vis = V_SmithGGXCorrelated(NdotV, NdotL, a);
	float3 F   = F_Schlick(HdotV, F0);

	float3 kD = (1.0 - F) * (1.0 - metallic);

	BRDFResult result;
	result.specular = D * Vis * F;
	result.diffuse  = kD * DiffuseOrenNayar(albedo, NdotL, NdotV, roughness, L, V, N);
	return result;
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

// Returns diffuse irradiance blended from all nearby SH probes.
// Falls back to slot 0 (skybox) when no scene probe covers worldPos.
float3 sampleSHProbes(float3 worldPos, float3 N)
{
    uint  count      = shProbeCount[0];
    float totalW     = 0.0f;
    float3 blended[9];
    for (int k = 0; k < 9; ++k) blended[k] = float3(0.0, 0.0, 0.0);

    for (uint i = 1; i < count; ++i)
    {
        float dist = length(worldPos - shProbes[i].position);
        if (dist >= shProbes[i].influenceRadius) continue;

        // Normalized distance [0,1], then smoothstep for C1 falloff
        float t = dist / shProbes[i].influenceRadius;
        float w = smoothstep(1.0, 0.0, t);

        for (int k = 0; k < 9; ++k)
            blended[k] += getProbeCoeff(i, k) * w;
        totalW += w;
    }

    if (totalW < 1e-4f)
    {
        // Outside all probe radii — fall back to skybox slot 0
        float3 sh[9];
        for (int k = 0; k < 9; ++k) sh[k] = getProbeCoeff(0, k);
        return computeIrradiance(N, sh);
    }

    float3 sh[9];
    for (int k = 0; k < 9; ++k) sh[k] = blended[k] / totalW;
    return computeIrradiance(N, sh);
}

struct FragOutput
{
	float4 color    : SV_Target0;
	float4 vsNormal : SV_Target1;
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

	float3 emissive =
	    textureArray[NonUniformResourceIndex(mat.emissiveIndex)].SampleGrad(input.fragTexCoord, dx, dy).rgb;
	emissive *= mat.emissiveFactor * mat.emissiveStrength;

	float4 mrSample =
	    textureArray[NonUniformResourceIndex(mat.metallicRoughnessIndex)].SampleGrad(input.fragTexCoord, dx, dy);
	float roughness = max(mrSample.g * mat.roughnessFactor, MIN_ROUGHNESS);
	float metallic  = saturate(mrSample.b * mat.metallicFactor);

	// === NORMAL MAPPING ===
	float3 N = normalize(input.fragNormal);
	if (!isFrontFace) N = -N;
	float3 T = normalize(input.fragTangent);
	float3 B = normalize(input.fragBitangent);

	float3 mapNormal  = SampleNormalMap(mat.normalMapIndex, input.fragTexCoord, dx, dy);
	float3 finalNormal = normalize(mapNormal.x * T + mapNormal.y * B + mapNormal.z * N);

	roughness = GeometricSpecularAA(finalNormal, roughness);

	// === LIGHTING SETUP ===
	float3 L = normalize(directionalLight[0].direction.xyz);
	float3 V = normalize(camera[0].cameraPositionAndPadding.xyz - input.fragWorldPos);

	float NdotV = max(dot(finalNormal, V), 1e-4);

	float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo.rgb, metallic);

	// === SHADOW (PCF 3x3) ===
	float2 shadowTexelSize = directionalLight[0].shadowMapSize.zw;
	float3 projCoords = input.shadowCoord.xyz / input.shadowCoord.w;
	projCoords.xy = projCoords.xy * 0.5 + 0.5;
	float shadow = ComputeShadow(projCoords, shadowTexelSize, finalNormal, L);

	// === DIRECTIONAL LIGHT ===
	float3 directionalLightRadiance = directionalLight[0].color.rgb * directionalLight[0].color.a;

	LightingVectors lv   = ComputeLightingVectors(L, V, finalNormal);
	BRDFResult      brdf = EvaluateBRDF(albedo.rgb, F0, metallic, roughness,
	                                    lv.NdotL, NdotV, lv.NdotH, lv.HdotV,
	                                    L, V, finalNormal);

	float3 directLight = (brdf.diffuse + brdf.specular) * directionalLightRadiance * lv.NdotL * shadow;

	// === POINT / SPOT LIGHTS ===
	float3 litColorFromLights = float3(0.0, 0.0, 0.0);
	for (uint i = 0; i < lightCount[0]; i++)
	{
		float3 Lp   = lights[i].position - input.fragWorldPos;
		float  dist = length(Lp);
		if (dist >= lights[i].radius) continue;

		float3 Ldir = Lp / dist;

		float spotAttenuation = 1.0;
		if (lights[i].type == 1)
		{
			float cosAngle = dot(-Ldir, lights[i].direction);
			spotAttenuation = smoothstep(lights[i].outerConeAngle, lights[i].innerConeAngle, cosAngle);
			if (spotAttenuation <= 0.0) continue;
		}

		float attenuation = PointLightAttenuation(dist, lights[i].radius);

		LightingVectors lvp   = ComputeLightingVectors(Ldir, V, finalNormal);
		BRDFResult      brdfp = EvaluateBRDF(albedo.rgb, F0, metallic, roughness,
		                                     lvp.NdotL, NdotV, lvp.NdotH, lvp.HdotV,
		                                     Ldir, V, finalNormal);

		float3 radiance = lights[i].color * lights[i].intensity;
		litColorFromLights += (brdfp.diffuse + brdfp.specular) * radiance * lvp.NdotL * attenuation * spotAttenuation;
	}

	// === IBL / Ambient ===
	float3 F_ibl = F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) * pow(1.0 - NdotV, 5.0);

    float3 ambient;
    float3 startAmbient;

    startAmbient = directionalLight[0].ambient.a * directionalLight[0].ambient.rgb;

	if (SKYBOX_ENABLED == 1)
    {
        float3 irradiance = sampleSHProbes(input.fragWorldPos, finalNormal);
		float3 iblDiffuse  = (1.0 - F_ibl) * (1.0 - metallic) * DiffuseOrenNayarIBL(albedo.rgb, NdotV, roughness) * irradiance;

		float3 iblSpecular = float3(0.0, 0.0, 0.0);
		if (mat.alphaMode != 1 && mat.doubleSided != 1)
		{
			float3 R            = reflect(-V, finalNormal);
			float3 prefiltColor = prefilteredMap.SampleLevel(R, roughness * MAX_REFLECTION_LOD).rgb;
			float2 brdfLut      = brdfLutMap.SampleLevel(float2(NdotV, roughness), 0).rg;
			iblSpecular         = prefiltColor * (F_ibl * brdfLut.x + brdfLut.y);
		}

        ambient = (iblDiffuse + iblSpecular) + startAmbient;
	}
	else
	{
		// Fallback without skybox
        ambient = (1.0 - F_ibl) * (1.0 - metallic)
		        * DiffuseOrenNayarIBL(albedo.rgb, NdotV, roughness)
		        * startAmbient;
	}
    float3 litColor = directLight + litColorFromLights + ambient + emissive;

	// === FINAL OUTPUT ===
	float outAlpha = (mat.alphaMode == 2) ? albedo.a : 1.0;

	float3 viewNormal = mul((float3x3)camera[0].viewMatrix, finalNormal);

	FragOutput output;
	output.color    = float4(litColor, outAlpha);
	output.vsNormal = float4(normalize(viewNormal), 1.0);
	return output;
}