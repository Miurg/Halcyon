// === SET 0 ===

struct SunUBO
{
	float4x4 lightSpaceMatrix;
	float4 sunDirection;
	float4 sunColor;   // .rgb = color, .a = intensity
	float4 sunAmbient; // .rgb = color, .a = intensity
	float4 shadowMapSize;
    float4 frustumPlanes[6];
    float shadowCasterRange;
    float3 _pad;
    float4 cameraFrustumLightSpaceBounds;
};
[[vk::binding(1, 0)]]
StructuredBuffer<SunUBO> sun;

// === SET 1 ===

struct ModelData
{
	float4 AABBMin;
	float4 AABBMax;
	uint transformIndex;
	uint materialIndex;
	uint drawCommandIndex; // unused
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

// === Vertex I/O ===

struct VSInput
{
	float3 inPosition;
};

struct VSOutput
{
	float4 pos : SV_Position;
};

[shader("vertex")]
VSOutput vertMain(VSInput input, uint instanceID: SV_InstanceID, uint baseInstance: SV_StartInstanceLocation)
{
    uint objectIndex = visibleIndicesBuffer[baseInstance + instanceID];
    ModelData model = objectBuffer[objectIndex];
	float4x4 modelMatrix = transformBuffer[model.transformIndex].model;

	float4 worldPos = mul(modelMatrix, float4(input.inPosition, 1.0));

	VSOutput output;
	output.pos = mul(sun[0].lightSpaceMatrix, worldPos);
	return output;
}
