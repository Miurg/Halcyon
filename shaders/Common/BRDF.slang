module BRDF;

import Common.Constants;

// GGX normal distribution function (Trowbridge & Reitz, 1975 / Walter et al., 2007).
//        a^2
// ──────────────────────────
// PI * ((NdotH^2 * (a^2 - 1) + 1)^2)
public float D_GGX(float NdotH, float a)
{
	float a2 = a * a;
	float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
	return a2 / (PI * d * d);
}

// Correlated Smith visibility function for GGX distribution (Heitz, 2014).
//                    0.5
// ──────────────────────────────────────────────
// NdotL*(NdotV*(1-a)+a) + NdotV*(NdotL*(1-a)+a)
public float V_SmithGGXCorrelated(float NdotV, float NdotL, float a)
{
	float ggxV = NdotL * (NdotV * (1.0 - a) + a);
	float ggxL = NdotV * (NdotL * (1.0 - a) + a);
	return 0.5 / (ggxV + ggxL + 1e-5);
}

// Schlick approximation of the Fresnel term (Schlick, 1994), fast exp2 form.
// F0 + (1 - F0) * 2^((-5.55473 * HdotV - 6.98316) * HdotV)
public float3 F_Schlick(float HdotV, float3 F0)
{
    return F0 + (1.0 - F0) * exp2((-5.55473 * HdotV - 6.98316) * HdotV);
}

// Roughness-aware Fresnel for IBL — uses max(F0, 1 - roughness) as the F90 ceiling
// so grazing-angle reflections wash out on rough surfaces.
// (Lazarov, 2013 / Lagarde & de Rousiers, 2014.)
public float3 F_SchlickRoughness(float NdotV, float3 F0, float roughness)
{
    return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) * pow(1.0 - NdotV, 5.0);
}

// "Generalization of Lambert's Reflectance Model" (Oren & Nayar, 1994).
public float3 DiffuseOrenNayar(float3 albedo, float NdotL, float NdotV, float roughness,
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

public float3 DiffuseOrenNayarIBL(float3 albedo, float NdotV, float roughness)
{
    float sigma2 = roughness * roughness;
    float A = 1.0 - 0.5 * sigma2 / (sigma2 + 0.57);
    // Without B, we dont have L in ambient
    return albedo / PI * A;
}

// Increases roughness based on normal map variance to reduce specular aliasing.
// "Stable Specular Highlights" (Tokuyoshi & Kaplanyan, 2019).
public float GeometricSpecularAA(float3 worldNormal, float roughness)
{
	float3 dndu = ddx(worldNormal);
	float3 dndv = ddy(worldNormal);

	float variance = 0.5 * (dot(dndu, dndu) + dot(dndv, dndv));

	float roughness2 = roughness * roughness;
	float kernelRoughness2 = min(2.0 * variance, 0.18);
	return sqrt(saturate(roughness2 + kernelRoughness2));
}

// Inverse-square falloff with a smooth windowing function (Karis, UE4): avoids the
// singularity at dist=0 and reaches exactly zero at the light radius.
public float PointLightAttenuation(float dist, float radius)
{
	float x = saturate(1.0 - pow(dist / radius, 4.0));
	return x * x / (dist * dist + 1.0);
}

public struct LightingVectors
{
	public float NdotL;
	public float NdotH;
	public float HdotV;
};

public LightingVectors ComputeLightingVectors(float3 L, float3 V, float3 N)
{
    LightingVectors lv;
    float3 H     = normalize(L + V);
	lv.NdotL = max(dot(N, L), 0.0);
	lv.NdotH = max(dot(N, H), 0.0);
	lv.HdotV = max(dot(H, V), 0.0);
	return lv;
}

public struct BRDFResult
{
	public float3 diffuse;
	public float3 specular;
};

// View-invariant per-fragment surface data — bundled to avoid passing
// 7+ arguments through every BRDF / light evaluation.
public struct Surface
{
	public float3 albedo;
	public float3 F0;
	public float  metallic;
	public float  roughness;
	public float3 N;
	public float3 V;
	public float  NdotV;
};

// Cook-Torrance for a single light: diffuse (Oren-Nayar) and specular (GGX/Smith) kept apart.
public BRDFResult EvaluateBRDF(Surface s, LightingVectors lv, float3 L)
{
	float a = s.roughness * s.roughness;

	float  D   = D_GGX(lv.NdotH, a);
	float  Vis = V_SmithGGXCorrelated(s.NdotV, lv.NdotL, a);
	float3 F   = F_Schlick(lv.HdotV, s.F0);

	float3 kD = (1.0 - F) * (1.0 - s.metallic);

	BRDFResult result;
	result.specular = D * Vis * F;
	result.diffuse  = kD * DiffuseOrenNayar(s.albedo, lv.NdotL, s.NdotV, s.roughness, L, s.V, s.N);
	return result;
}

// Returns zero contribution outside the light's radius or spot cone.
// Light parameters are passed field by field so this module stays free of GPU struct definitions.
public float3 EvaluatePunctualLight(Surface s, float3 worldPos, float3 lightPos, float radius, float3 lightColor,
                                    float intensity, float3 spotDir, float innerConeAngle, float outerConeAngle,
                                    uint type)
{
	float3 Lp   = lightPos - worldPos;
	float  dist = length(Lp);
	if (dist >= radius) return float3(0.0);

	float3 Ldir = Lp / dist;

	float spotAttenuation = 1.0;
	if (type == 1)
	{
		float cosAngle = dot(-Ldir, spotDir);
		spotAttenuation = smoothstep(outerConeAngle, innerConeAngle, cosAngle);
		if (spotAttenuation <= 0.0) return float3(0.0);
	}

	float attenuation = PointLightAttenuation(dist, radius);

	LightingVectors lv   = ComputeLightingVectors(Ldir, s.V, s.N);
	BRDFResult      brdf = EvaluateBRDF(s, lv, Ldir);

	float3 radiance = lightColor * intensity;
	return (brdf.diffuse + brdf.specular) * radiance * lv.NdotL * attenuation * spotAttenuation;
}

public float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{
	float a = roughness * roughness;

	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	float3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;

	float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
	float3 tangent = normalize(cross(up, N));
	float3 bitangent = cross(N, tangent);

	return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}

// Split-sum IBL uses the k = a^2/2 remap, not the direct-lighting one.
public float GeometrySchlickGGX(float NdotV, float roughness)
{
    float a = roughness;
    float k = (a * a) / 2.0;
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}

public float GeometrySmith(float NdotV, float NdotL, float roughness)
{
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}
