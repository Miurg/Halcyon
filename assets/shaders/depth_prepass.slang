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
[[vk::binding(0, 0)]] StructuredBuffer<CameraData> camera;

// === SET 1 ===

struct ModelData
{
    float4 AABBMin;
    float4 AABBMax;
    uint   transformIndex;
    uint   materialIndex;
    uint   drawCommandIndex; // unused
};
[[vk::binding(0, 1)]] StructuredBuffer<ModelData> objectBuffer;

struct TransformData
{
    float4x4 model;
};
[[vk::binding(1, 1)]] StructuredBuffer<TransformData> transformBuffer;

[[vk::binding(3, 1)]] StructuredBuffer<uint> visibleIndicesBuffer;

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
VSOutput vertMain(VSInput input, uint instanceID : SV_InstanceID, uint baseInstance : SV_StartInstanceLocation)
{
    uint objectIndex = visibleIndicesBuffer[baseInstance + instanceID];
    ModelData model = objectBuffer[objectIndex];
    float4x4 modelMatrix = transformBuffer[model.transformIndex].model;

    float4 worldPos = mul(modelMatrix, float4(input.inPosition, 1.0));

    VSOutput output;
    output.pos = mul(camera[0].cameraSpaceMatrix, worldPos);
    return output;
}