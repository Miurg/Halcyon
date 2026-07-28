#include "Shared/GpuStructs.h"
#include "Shared/Bindings.h"

// === SET 0 ===

[[vk::binding(BIND_GLOBAL_SUN, 0)]]
StructuredBuffer<DirectionalLightData> directionalLight;

// === SET 1 ===

[[vk::binding(BIND_MODEL_PRIMITIVES, 1)]]
StructuredBuffer<ModelData> objectBuffer;

[[vk::binding(BIND_MODEL_TRANSFORMS, 1)]]
StructuredBuffer<TransformData> transformBuffer;

[[vk::binding(BIND_MODEL_VISIBLE_INDICES, 1)]]
StructuredBuffer<uint> visibleIndicesBuffer;

// === SET 2 ===

[[vk::binding(BIND_TEXTURES_ARRAY, 2)]]
Sampler2D textureArray[MAX_BINDLESS_TEXTURES];

[[vk::binding(BIND_TEXTURES_MATERIALS, 2)]]
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
