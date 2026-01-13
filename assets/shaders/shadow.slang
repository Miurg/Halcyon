struct ModelData
{
    float4x4 model;
};
[[vk::binding(0, 2)]] StructuredBuffer<ModelData> objectBuffer;

struct CameraUBO
{
    float4x4 view;
    float4x4 proj;
    float4x4 lightSpaceMatrix;
};
[[vk::binding(0, 0)]] StructuredBuffer<CameraUBO> lightCamera; 


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
    
    float4 worldPos = mul(float4(input.inPosition, 1.0), data.model);

    VSOutput output;
    output.pos = mul(lightCamera[0].lightSpaceMatrix, worldPos);
    
    return output;
}
