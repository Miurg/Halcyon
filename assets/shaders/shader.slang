struct ModelData
{
    float4x4 model;
    uint textureIndex;
};
[[vk::binding(0, 2)]] StructuredBuffer<ModelData> objectBuffer;

struct CameraData
{
    float4x4 cameraSpaceMatrix;
};
[[vk::binding(0, 0)]] StructuredBuffer<CameraData> camera;
[[vk::binding(1, 0)]] Sampler2DShadow shadowMap;

struct SunData
{
    float4x4 lightSpaceMatrix;
};
[[vk::binding(2, 0)]] StructuredBuffer<SunData> sun;

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
    uint textureIndex;
};

[shader("vertex")]
VSOutput vertMain(VSInput input, uint instanceID: SV_InstanceID, uint baseInstance: SV_StartInstanceLocation)
{
    ModelData data = objectBuffer[instanceID + baseInstance];
    VSOutput output;

    float4 worldPos = mul(float4(input.inPosition, 1.0), data.model);

    output.pos = mul(camera[0].cameraSpaceMatrix, worldPos);

    output.shadowCoord = mul(sun[0].lightSpaceMatrix, worldPos);

    output.fragColor = input.inColor;
    output.fragTexCoord = input.inTexCoord;
    output.textureIndex = data.textureIndex;
    return output;
}
[[vk::binding(0, 1)]] Sampler2D textureArray[1024];

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET
{
    float3 color = textureArray[vertIn.textureIndex].Sample(vertIn.fragTexCoord).rgb;

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