struct ModelData
{
    float4x4 model;
    uint textureIndex;
    uint _padding[3];
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
    float4 sunDirection;
    float4 sunColor;
    float4 sunAmbient;
};
[[vk::binding(2, 0)]] StructuredBuffer<SunData> sun;

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
    float3 fragColor;
    float3 fragNormal;
    float2 fragTexCoord;
    float4 shadowCoord;
    nointerpolation uint textureIndex;
};

[shader("vertex")]
VSOutput vertMain(VSInput input, uint instanceID: SV_InstanceID, uint baseInstance: SV_StartInstanceLocation)
{
    ModelData data = objectBuffer[instanceID + baseInstance];
    VSOutput output;

    float4 worldPos = mul(data.model, float4(input.inPosition, 1.0));

    output.pos = mul(camera[0].cameraSpaceMatrix, worldPos);

    output.shadowCoord = mul(sun[0].lightSpaceMatrix, worldPos);

    output.fragColor = input.inColor;
    output.fragTexCoord = input.inTexCoord; 
    output.fragNormal = normalize(mul((float3x3)data.model, input.inNormal));
    output.textureIndex = data.textureIndex;
    return output;
}
[[vk::binding(0, 1)]] Sampler2D textureArray[1024];

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET
{
    float2 dx = ddx(vertIn.fragTexCoord);
    float2 dy = ddy(vertIn.fragTexCoord);
    float4 albedo = textureArray[NonUniformResourceIndex(vertIn.textureIndex)].SampleGrad(vertIn.fragTexCoord, dx, dy);
    if (albedo.a < 0.1) discard;

    float3 norm = normalize(vertIn.fragNormal);

    float3 lightDir = normalize(sun[0].sunDirection.xyz);

    float3 projCoords = vertIn.shadowCoord.xyz / vertIn.shadowCoord.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    float shadowFactor = 1.0;
    float bias = 0.001;

    if (saturate(projCoords.x) == projCoords.x && 
        saturate(projCoords.y) == projCoords.y && 
        projCoords.z > 0.0 && projCoords.z < 1.0)
    {
        shadowFactor = shadowMap.SampleCmpLevelZero(projCoords.xy, projCoords.z - bias);
    }

    float diff = max(dot(norm, lightDir), 0.0);

    float3 sunLight = sun[0].sunColor.rgb * sun[0].sunColor.a * diff * shadowFactor;
    float3 ambientLight = sun[0].sunAmbient.rgb * sun[0].sunAmbient.a;
    float3 finalColor = (sunLight + ambientLight) * albedo.rgb * vertIn.fragColor;

    return float4(finalColor, albedo.a);
}