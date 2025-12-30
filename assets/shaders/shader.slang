struct ModelData
{
    float4x4 model;
};
[[vk::binding(0, 2)]] StructuredBuffer<ModelData> objectBuffer;

struct CameraUBO
{
    float4x4 view;
    float4x4 proj;
};
[[vk::binding(0, 0)]] ConstantBuffer<CameraUBO> camera;

struct VSInput
{
    float3 inPosition;
    float3 inColor;
    float2 inTexCoord;
};

struct VSOutput
{
    float4 pos : SV_Position;
    float3 fragColor;
    float2 fragTexCoord;
};

struct PushConsts
{
    uint objectIndex;
};
[[vk::push_constant]] ConstantBuffer<PushConsts> push;

[shader("vertex")]
VSOutput vertMain(VSInput input)
{
    ModelData data = objectBuffer[push.objectIndex];
    VSOutput output;
    float4 worldPos = mul(float4(input.inPosition, 1.0), data.model);

    output.pos = mul(camera.proj, mul(camera.view, worldPos));
    output.fragColor = input.inColor;
    output.fragTexCoord = input.inTexCoord;
    return output;
}
[[vk::binding(0, 1)]] Sampler2D texture;

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET
{
    return texture.Sample(vertIn.fragTexCoord);
}