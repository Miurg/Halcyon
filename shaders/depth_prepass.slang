#include "Shared/GpuStructs.h"
#include "Shared/Bindings.h"

// === SET 0 ===

[[vk::binding(BIND_GLOBAL_CAMERA, 0)]]
StructuredBuffer<CameraData> camera;

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

// === Spec constants ===

[[vk::constant_id(0)]]
const int ALPHA_TEST_ENABLED = 0;

[[vk::constant_id(1)]]
const int ALPHA_TO_COVERAGE = 0;

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
    half2 fragTexCoord;
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
    output.fragTexCoord = (half2)input.inTexCoord;
	output.materialIndex = model.materialIndex;
	return output;
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target0
{
	float coverage = 1.0;
	if (ALPHA_TEST_ENABLED == 1)
	{
		MaterialData mat = materialBuffer[input.materialIndex];
		if (mat.alphaMode != 1) discard;
		float alpha = textureArray[NonUniformResourceIndex(mat.textureIndex)]
		                  .Sample(input.fragTexCoord).a * mat.baseColorFactor.a;
		if (ALPHA_TO_COVERAGE == 0)
		{
			if (alpha < mat.alphaCutoff) discard;
		}
		else
		{
			// Wyman & McGuire, "Anti-Aliased Alpha Test", JCGT 2017.
			coverage = saturate((alpha - mat.alphaCutoff) / max(fwidth(alpha), 1e-5) + 0.5);
			if (coverage <= 0.0) discard;
		}
	}
	return float4(normalize(input.viewNormal), coverage);
}
