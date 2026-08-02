module Shadow;

// 3x3 PCF with a hardware comparison sampler; normal-based bias against acne at grazing angles.
// Returns 1.0 for fragments outside the shadow map.
public float ComputeShadow(Sampler2DShadow shadowMap, float4 shadowCoord, float2 texelSize, float3 surfaceNormal,
                           float3 lightDir)
{
	float bias = max(0.0005 * (1.0 - dot(surfaceNormal, lightDir)), 0.00005);

	return ComputeShadowPCF(shadowMap, shadowCoord, texelSize, bias);
}

public float ComputeShadowPCF(Sampler2DShadow shadowMap, float4 shadowCoord, float2 texelSize, float bias)
{
	float3 p = shadowCoord.xyz / shadowCoord.w;
	p.xy = p.xy * 0.5 + 0.5;

	if (any(p.xy < 0.0) || any(p.xy > 1.0) || p.z <= 0.0 || p.z >= 1.0) return 1.0;

    float2 texelPos = p.xy / texelSize - 0.5;
    float2 base = floor(texelPos);
    float2 f = frac(texelPos);

    float2 w0 = 2.0 - f;
    float2 w1 = 1.0 + f;

    float2 uv0 = (base - 1.0 + rcp(w0) + 0.5) * texelSize;
    float2 uv1 = (base + 1.0 + f / w1 + 0.5) * texelSize;

    float compareDepth = p.z + bias;

    float visibility =
    shadowMap.SampleCmpLevelZero(float2(uv0.x, uv0.y), compareDepth) * w0.x * w0.y +
                 shadowMap.SampleCmpLevelZero(float2(uv1.x, uv0.y), compareDepth) * w1.x * w0.y +
                 shadowMap.SampleCmpLevelZero(float2(uv0.x, uv1.y), compareDepth) * w0.x * w1.y +
                 shadowMap.SampleCmpLevelZero(float2(uv1.x, uv1.y), compareDepth) * w1.x * w1.y;

    return visibility / 9.0;
}
