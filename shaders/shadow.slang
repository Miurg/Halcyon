// === SET 0 ===

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

// === CONST ===

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
	float2 fragTexCoord;
	nointerpolation uint materialIndex;
};

[shader("vertex")]
VSOutput vertMain(VSInput input, uint instanceID: SV_InstanceID, uint baseInstance: SV_StartInstanceLocation)
{
    uint objectIndex = visibleIndicesBuffer[baseInstance + instanceID];
    ModelData model = objectBuffer[objectIndex];
	float4x4 modelMatrix = transformBuffer[model.transformIndex].model;

	float4 worldPos = mul(modelMatrix, float4(input.inPosition, 1.0));

    VSOutput output;
    output.pos = mul(directionalLight[0].lightSpaceMatrix, worldPos);
	output.fragTexCoord = input.inTexCoord;
	output.materialIndex = model.materialIndex;
	return output;
}

[shader("fragment")]
void fragMain(VSOutput input)
{
	if (ALPHA_TEST_ENABLED == 1)
	{
		MaterialData mat = materialBuffer[input.materialIndex];
		if (mat.alphaMode == 1)
		{
			float a = textureArray[NonUniformResourceIndex(mat.textureIndex)].Sample(input.fragTexCoord).a
			        * mat.baseColorFactor.a;
			if (a < mat.alphaCutoff) discard;
		}
	}
}
