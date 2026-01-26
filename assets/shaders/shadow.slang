struct ModelData
{
    uint transformIndex;
    uint textureIndex;
    uint _padding[2];
};
[[vk::binding(0, 2)]] StructuredBuffer<ModelData> objectBuffer;

struct TransformData
{
    float4x4 model;
};
[[vk::binding(1, 2)]] StructuredBuffer<TransformData> transformBuffer;

struct SunUBO
{
    float4x4 lightSpaceMatrix;
    float4 sunDirection;
    float4 sunColor;
    float4 sunAmbient;
};
[[vk::binding(2, 0)]] StructuredBuffer<SunUBO> sun;

struct VSInput
{
    float3 inPosition;
};

struct VSOutput
{
    float4 pos : SV_Position;
};

[shader("vertex")]
VSOutput vertShadow(VSInput input, uint instanceID: SV_InstanceID, uint baseInstance: SV_StartInstanceLocation)
{
    ModelData data = objectBuffer[instanceID + baseInstance];
    TransformData transform = transformBuffer[data.transformIndex];

    float4 worldPos = mul(transform.model, float4(input.inPosition, 1.0));

    VSOutput output;
    output.pos = mul(sun[0].lightSpaceMatrix, worldPos);
    
    return output;
}
