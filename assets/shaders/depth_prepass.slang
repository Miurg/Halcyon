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

// === SET 2 ===
[[vk::binding(0, 2)]]
Sampler2D textureArray[2048];

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
	float4 baseColorFactor;
	float3 emissiveFactor;
	float emissivePadding;
	float roughnessFactor;
	float metallicFactor;
	float padding1;
	float padding2;
};
[[vk::binding(2, 2)]]
StructuredBuffer<MaterialData> materialBuffer;

// === Spec constants ===

[[vk::constant_id(0)]]
const int ALPHA_TEST_ENABLED = 0;

// === Vertex I/O ===

struct VSInput
{
	float3 inPosition;
	float3 inColor;
	float3 inNormal;
	float2 inTexCoord;
};

struct VSOutput
{
	float4 pos : SV_Position;
	float3 viewNormal;
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

	float3x3 normalMatrix = float3x3(
		cross(modelMatrix3x3[1], modelMatrix3x3[2]),
		cross(modelMatrix3x3[2], modelMatrix3x3[0]),
		cross(modelMatrix3x3[0], modelMatrix3x3[1]));
	float3 worldNormal = normalize(mul(normalMatrix, input.inNormal));

	VSOutput output;
	output.pos = mul(camera[0].cameraSpaceMatrix, worldPos);
	output.viewNormal = mul((float3x3)camera[0].viewMatrix, worldNormal);
	output.fragTexCoord = input.inTexCoord;
	output.materialIndex = model.materialIndex;
	return output;
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target0
{
	if (ALPHA_TEST_ENABLED == 1)
	{
		MaterialData mat = materialBuffer[input.materialIndex];
		if (mat.alphaMode != 1) discard;
		float alpha = textureArray[NonUniformResourceIndex(mat.textureIndex)]
		                  .Sample(input.fragTexCoord).a * mat.baseColorFactor.a;
		if (alpha < mat.alphaCutoff) discard;
	}
	return float4(normalize(input.viewNormal), 1.0);
}
