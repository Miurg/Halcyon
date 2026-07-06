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

struct BakePush
{
	float3 probePos;
	uint faceIdx;
};
[[vk::push_constant]]
BakePush bakePush;

// Must match the face tables in global_illumination_forward.slang
static const float3 kFaceForward[6] = {
	float3(1, 0, 0), float3(-1, 0, 0), float3(0, 1, 0), float3(0, -1, 0), float3(0, 0, 1), float3(0, 0, -1),
};
static const float3 kFaceUp[6] = {
	float3(0, 1, 0), float3(0, 1, 0), float3(0, 0, -1), float3(0, 0, 1), float3(0, 1, 0), float3(0, 1, 0),
};

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

	float3 f = kFaceForward[bakePush.faceIdx];
	float3 s = normalize(cross(f, kFaceUp[bakePush.faceIdx]));
	float3 u = cross(s, f);
	float2 ndc = uv * 2.0 - 1.0;

	VSOutput output;
	output.pos = pos;
	// 90-degree face frustum: ray direction straight from the face basis, no invViewProj needed
	output.localPos = ndc.x * s - ndc.y * u + f;

	return output;
}

struct FragOutput
{
	float4 color : SV_Target0;
};

[shader("fragment")]
FragOutput fragMain(VSOutput input)
{
    FragOutput output;

    float3 viewDir = normalize(input.localPos);
    float3 envColor = skyboxMap.Sample(viewDir).rgb;

    output.color = float4(envColor, 1.0);

    return output;
}
