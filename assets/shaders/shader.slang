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
[[vk::binding(0, 0)]] StructuredBuffer<CameraUBO> camera;
[[vk::binding(1, 0)]] Sampler2DShadow shadowMap;

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
    float4 shadowCoord;
};

struct PushConsts
{
    uint objectIndex;
    uint textureIndex;
};
[[vk::push_constant]] ConstantBuffer<PushConsts> push;

[shader("vertex")]
VSOutput vertMain(VSInput input)
{
    ModelData data = objectBuffer[push.objectIndex];
    VSOutput output;

    float4 worldPos = mul(float4(input.inPosition, 1.0), data.model);

    output.pos = mul(camera[0].proj, mul(camera[0].view, worldPos));

    output.shadowCoord = mul(camera[0].lightSpaceMatrix, worldPos);

    output.fragColor = input.inColor;
    output.fragTexCoord = input.inTexCoord;
    return output;
}
[[vk::binding(0, 1)]] Sampler2D textureArray[1024];

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET
{
    float3 color = textureArray[push.textureIndex].Sample(vertIn.fragTexCoord).rgb;

    float3 projCoords = vertIn.shadowCoord.xyz / vertIn.shadowCoord.w;

    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    float shadowFactor = 0.0;

    if (projCoords.z > 0.0 && projCoords.z < 1.0 && 
        projCoords.x > 0.0 && projCoords.x < 1.0 && 
        projCoords.y > 0.0 && projCoords.y < 1.0)
    {
        shadowFactor = shadowMap.SampleCmpLevelZero(projCoords.xy, projCoords.z);
    }
    else
    {
        shadowFactor = 1.0;
    }

    float lightIntensity = 0.2 + (shadowFactor * 0.8);
    return float4(color * lightIntensity, 1.0);
}