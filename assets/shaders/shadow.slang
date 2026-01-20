struct ModelData
{
    float4x4 model;
    uint textureIndex;
};
[[vk::binding(0, 2)]] StructuredBuffer<ModelData> objectBuffer;

struct SunUBO
{
    float4x4 lightSpaceMatrix;
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

    float4 worldPos = mul(data.model, float4(input.inPosition, 1.0));

    VSOutput output;
    output.pos = mul(sun[0].lightSpaceMatrix, worldPos);
    
    return output;
}
