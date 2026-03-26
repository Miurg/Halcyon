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
    float3 emissiveFactor;
    float emissivePadding;
	float roughnessFactor;
	float metallicFactor;
	float padding1;
	float padding2;
};
[[vk::binding(2, 2)]]
StructuredBuffer<MaterialData> materialBuffer;

[[vk::binding(5, 2)]]
SamplerCube irradianceMap;
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

	float3x3 normalMatrix = (float3x3)modelMatrix;

	float3 N = normalize(mul(normalMatrix, input.inNormal));
	float3 T = normalize(mul(normalMatrix, input.inTangent.xyz));
	T = normalize(T - dot(T, N) * N);
	float3 B = cross(N, T) * input.inTangent.w;

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

// Helper func
float3 SampleNormalMap(uint mapIndex, float2 uv, float2 dx, float2 dy)
{
	float3 raw = textureArray[NonUniformResourceIndex(mapIndex)].SampleGrad(uv, dx, dy).rgb;
	return normalize(raw * 2.0 - 1.0);
}

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

float D_GGX(float NdotH, float a)
{
	float a2 = a * a;
	float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
	return a2 / (PI * d * d);
}

float V_SmithGGXCorrelated(float NdotV, float NdotL, float a)
{
	float ggxV = NdotL * (NdotV * (1.0 - a) + a);
	float ggxL = NdotV * (NdotL * (1.0 - a) + a);
	return 0.5 / (ggxV + ggxL + 1e-5);
}

float3 F_Schlick(float cosTheta, float3 F0)
{
	return F0 + (1.0 - F0) * exp2((-5.55473 * cosTheta - 6.98316) * cosTheta);
}

float3 ApplyFog(float3 color, float viewDepth)
{
	const float fogStart = 2000.0;
	const float fogEnd = 15000.0;
	const float3 fogColor = float3(0.0, 0.7, 0.8);

	float factor = saturate((viewDepth - fogStart) / (fogEnd - fogStart));
	return lerp(color, fogColor, factor);
}

float3 DiffuseLambert(float3 albedo)
{
    return albedo / PI;
}

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

float3 DiffuseDisney(float3 albedo, float NdotL, float NdotV, float HdotV, float roughness)
{
    float FL   = pow(1.0 - NdotL, 5.0);
    float FV   = pow(1.0 - NdotV, 5.0);
    float Fd90 = 0.5 + 2.0 * HdotV * HdotV * roughness;
    float Fd   = lerp(1.0, Fd90, FL) * lerp(1.0, Fd90, FV);
    return albedo / PI * Fd;
}

float PointLightAttenuation(float dist, float radius)
{
    float x = saturate(1.0 - pow(dist / radius, 4.0));
    return x * x / (dist * dist + 1.0);
}

struct FragOutput
{
	float4 color : SV_Target0;
	float4 vsNormal : SV_Target1;
};

[shader("fragment")]
FragOutput fragMain(VSOutput input, bool isFrontFace: SV_IsFrontFace)
{
	MaterialData mat = materialBuffer[input.materialIndex];

	float2 dx = ddx(input.fragTexCoord);
	float2 dy = ddy(input.fragTexCoord);

	// === MATERIAL SAMPLING ===
	float4 albedo = textureArray[NonUniformResourceIndex(mat.textureIndex)].SampleGrad(input.fragTexCoord, dx, dy) * float4(input.fragColor.xyz, 1.0);

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
	float metallic = saturate(mrSample.b * mat.metallicFactor);

	// === NORMAL MAPPING ===
    float3 N = normalize(input.fragNormal);
    if (!isFrontFace) N = -N;
	float3 T = normalize(input.fragTangent);
	float3 B = normalize(input.fragBitangent);

	float3 mapNormal = SampleNormalMap(mat.normalMapIndex, input.fragTexCoord, dx, dy);
	float3 finalNormal = normalize(mapNormal.x * T + mapNormal.y * B + mapNormal.z * N);

    // === LIGHTING SETUP ===
    float3 L = normalize(directionalLight[0].direction.xyz);
	float3 V = normalize(camera[0].cameraPositionAndPadding.xyz - input.fragWorldPos);
	float3 H = normalize(L + V);

    float NdotL = max(dot(finalNormal, L), 1e-4);
	float NdotV = max(dot(finalNormal, V), 1e-4);
	float NdotH = max(dot(finalNormal, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    // === SHADOW (PCF 3x3) ===
    float2 shadowTexelSize = directionalLight[0].shadowMapSize.zw;

	float3 projCoords = input.shadowCoord.xyz / input.shadowCoord.w;
	projCoords.xy = projCoords.xy * 0.5 + 0.5;
	float shadow = ComputeShadow(projCoords, shadowTexelSize, finalNormal, L);

	// === PBR (Cook-Torrance) ===
	float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo.rgb, metallic);

	float a = roughness * roughness;
	float D = D_GGX(NdotH, a);
	float Vis = V_SmithGGXCorrelated(NdotV, NdotL, a);
	float3 F = F_Schlick(HdotV, F0);
	float3 specular = D * Vis * F;

	float3 kD = (1.0 - F) * (1.0 - metallic);
	float3 diffuse = kD * DiffuseOrenNayar(albedo.rgb, NdotL, NdotV, roughness, L, V, finalNormal);

    float3 directionalLightRadiance = directionalLight[0].color.rgb * directionalLight[0].color.a;

    // === LIGHTS ===
    float3 litColorFromLights = float3(0.0, 0.0, 0.0);
    for (uint i = 0; i < lightCount[0]; i++)
    {
        float3 Lp = lights[i].position - input.fragWorldPos;
        float dist = length(Lp);
        if (dist >= lights[i].radius) continue;

        float3 Ldir = Lp / dist;
        float3 Hp = normalize(Ldir + V);

        float NdotLp = max(dot(finalNormal, Ldir), 1e-4);
        float NdotHp = max(dot(finalNormal, Hp), 0.0);
        float HdotVp = max(dot(Hp, V), 0.0);

        float attenuation = PointLightAttenuation(dist, lights[i].radius);

        float spotAttenuation = 1.0;
        if (lights[i].type == 1)
        {
            float cosAngle = dot(-Ldir, lights[i].direction);
            spotAttenuation = smoothstep(lights[i].outerConeAngle, lights[i].innerConeAngle, cosAngle);
            if (spotAttenuation <= 0.0) continue;
        }

        float ap = roughness * roughness;
        float Dp = D_GGX(NdotHp, ap);
        float Vp = V_SmithGGXCorrelated(NdotV, NdotLp, ap);
        float3 Fp = F_Schlick(HdotVp, F0);
        float3 specp = Dp * Vp * Fp;

        float3 kDp = (1.0 - Fp) * (1.0 - metallic);
        float3 diffp = kDp * DiffuseOrenNayar(albedo.rgb, NdotLp, NdotV, roughness, Ldir, V, finalNormal);

        float3 radiance = lights[i].color * lights[i].intensity;
        litColorFromLights += (diffp + specp) * radiance * NdotLp * attenuation * spotAttenuation;
    }
    // === IBL ===
    float3 F_ibl = F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) * pow(1.0 - NdotV, 5.0);

    float3 irradiance = irradianceMap.Sample(finalNormal).rgb;
    float3 iblDiffuse = (1.0 - F_ibl) * (1.0 - metallic) * albedo.rgb * irradiance;

    float3 iblSpecular = float3(0.0, 0.0, 0.0);
    if (mat.alphaMode != 1 || mat.doubleSided != 0) // mostly for foliage
    {
        float3 R = reflect(-V, finalNormal);
        float3 prefilteredColor = prefilteredMap.SampleLevel(R, roughness * MAX_REFLECTION_LOD).rgb;
        float2 brdf = brdfLutMap.SampleLevel(float2(NdotV, roughness), 0).rg;
        iblSpecular = prefilteredColor * (F_ibl * brdf.x + brdf.y);
    }

    float3 ambient = (iblDiffuse + iblSpecular) * directionalLight[0].ambient.a;

    float3 litColor = (diffuse + specular) * directionalLightRadiance * NdotL * shadow + litColorFromLights + ambient + emissive;

	// === FINAL OUTPUT ===
	float outAlpha = (mat.alphaMode == 2) ? albedo.a : 1.0;

	float3 viewNormal = mul((float3x3)camera[0].viewMatrix, finalNormal);

	FragOutput output;
	output.color = float4(ApplyFog(litColor, input.viewDepth), outAlpha);
	output.vsNormal = float4(normalize(viewNormal), 1.0);
	return output;
}