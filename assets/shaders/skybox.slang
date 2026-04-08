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
    float4 shadowMapSize;
    float4 frustumPlanes[6];
    float shadowCasterRange;
    float3 _pad;
    float4 cameraFrustumLightSpaceBounds;
};
[[vk::binding(1, 0)]]
StructuredBuffer<DirectionalLightData> directionalLight;

[[vk::binding(3, 2)]]
SamplerCube skyboxMap;

struct VSOutput
{
	float4 pos : SV_Position;
	float3 localPos;
};

[shader("vertex")]
VSOutput vertMain(uint vertexID: SV_VertexID)
{
	// Generate a fullscreen triangle on the far plane (z = 0.0, reverse-Z)
	float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
	float4 pos = float4(uv * 2.0 - 1.0, 0.0, 1.0);

	VSOutput output;
	output.pos = pos;
	// We map clip space to world space to get ray direction for sampling the skybox
	float4 target = mul(camera[0].invViewProj, pos);
	output.localPos = target.xyz / target.w - camera[0].cameraPositionAndPadding.xyz;

	return output;
}

float3 ComputeAnalyticalSun(float3 viewDir, float3 lightDir, float3 sunColor, float sunIntensity)
{
    float cosTheta = dot(viewDir, lightDir);

    float sunAngularRadius = 0.9998;
    float sunGlowRadius = 0.9980;

    float sunDisc = smoothstep(sunAngularRadius - 0.0001, sunAngularRadius + 0.0001, cosTheta);
    float sunGlow = smoothstep(sunGlowRadius, 1.0, cosTheta);
    sunGlow = pow(sunGlow, 3.0);

    float totalSun = max(sunDisc, sunGlow * 0.4);

    return sunColor * totalSun * sunIntensity;
}

struct FragOutput
{
	float4 color : SV_Target0;
	float4 normals : SV_Target1;
};

[shader("fragment")]
FragOutput fragMain(VSOutput input)
{
    FragOutput output;

    // Just in case
    float3 viewDir = normalize(input.localPos);
    float3 lightDir = normalize(directionalLight[0].direction.xyz);
    float3 sunColor = directionalLight[0].color.rgb;
    float sunIntensity = directionalLight[0].color.a * 100.0f;
    float3 sun = ComputeAnalyticalSun(viewDir, lightDir, sunColor, sunIntensity);

    float3 envColor = skyboxMap.Sample(input.localPos).rgb;
    
    output.color = float4(envColor + sun, 1.0);
    output.normals = float4(0.0, 0.0, 0.0, 0.0); // Don't disrupt normals target

    return output;
}
