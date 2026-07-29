module Shadow;

// 3x3 PCF with a hardware comparison sampler; normal-based bias against acne at grazing angles.
// Returns 1.0 for fragments outside the shadow map.
public float ComputeShadow(Sampler2DShadow shadowMap, float4 shadowCoord, float2 texelSize, float3 surfaceNormal,
                           float3 lightDir)
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
