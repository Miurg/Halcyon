struct ModelData
{
    float4 colorFactor;
    float4 AABBMin;
    float4 AABBMax;
    uint transformIndex;
    uint textureIndex;
    float2 padding;
};
[[vk::binding(0, 2)]] StructuredBuffer<ModelData> objectBuffer;

struct TransformData
{
    float4x4 model;
};
[[vk::binding(1, 2)]] StructuredBuffer<TransformData> transformBuffer;

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
    nointerpolation float4 colorFactor;
    float viewDepth;
};

[shader("vertex")]
VSOutput vertMain(VSInput input, uint instanceID: SV_InstanceID, uint baseInstance: SV_StartInstanceLocation)
{
    ModelData data = objectBuffer[instanceID + baseInstance];
    TransformData transform = transformBuffer[data.transformIndex];
    VSOutput output;

    float4 worldPos = mul(transform.model, float4(input.inPosition, 1.0));

    output.pos = mul(camera[0].cameraSpaceMatrix, worldPos);
    output.viewDepth = output.pos.w;
    output.shadowCoord = mul(sun[0].lightSpaceMatrix, worldPos);

    output.fragColor = input.inColor;
    output.fragTexCoord = input.inTexCoord;
    output.fragNormal = normalize(mul((float3x3)transform.model, input.inNormal));
    output.textureIndex = data.textureIndex;
    output.colorFactor = data.colorFactor;
    return output;
}
[[vk::binding(0, 1)]] Sampler2D textureArray[2048];

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET
{
    float2 dx = ddx(vertIn.fragTexCoord);
    float2 dy = ddy(vertIn.fragTexCoord);
    float4 textureColor = textureArray[NonUniformResourceIndex(vertIn.textureIndex)].SampleGrad(vertIn.fragTexCoord, dx, dy);
    float4 albedo = textureColor * vertIn.colorFactor;

    if (albedo.a < 0.1) discard;

    float3 norm = normalize(vertIn.fragNormal);

    float3 lightDir = normalize(sun[0].sunDirection.xyz);

    float3 projCoords = vertIn.shadowCoord.xyz / vertIn.shadowCoord.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    float shadowFactor = 0.0;
    // Calculate bias based on surface slope to prevent shadow acne
    // maximize bias when surface is parallel to light, minimize when perpendicular
    float bias = max(0.0005 * (1.0 - dot(norm, lightDir)), 0.00005); 

    if (saturate(projCoords.x) == projCoords.x && 
        saturate(projCoords.y) == projCoords.y && 
        projCoords.z > 0.0 && projCoords.z < 1.0)
    {
        // Get shadow map dimensions to determine texel size
        uint width, height, numLevels;
        shadowMap.GetDimensions(0, width, height, numLevels);
        float2 texelSize = 1.0 / float2(width, height);

        // 3x3 PCF Kernel
        for (int x = -1; x <= 1; ++x)
        {
            for (int y = -1; y <= 1; ++y)
            {
                // Sample shadow map with offset
                shadowFactor += shadowMap.SampleCmpLevelZero(projCoords.xy + float2(x, y) * texelSize, projCoords.z - bias);
            }
        }
        shadowFactor /= 9.0;
    }
    else // Outside of shadow map, assume not in shadow
    {
       
        shadowFactor = 1.0;
    }

    float diff = max(dot(norm, lightDir), 0.0);

    float3 sunLight = sun[0].sunColor.rgb * sun[0].sunColor.a * diff * shadowFactor;
    float3 ambientLight = sun[0].sunAmbient.rgb * sun[0].sunAmbient.a;
    float3 finalColor = (sunLight + ambientLight) * albedo.rgb * vertIn.fragColor;

    float fogStart = 20.0;
    float fogEnd = 150.0;
    float3 fogColor = float3(0.0, 0.7, 0.8);

    float fogFactor = saturate((vertIn.viewDepth - fogStart) / (fogEnd - fogStart));

    finalColor = lerp(finalColor, fogColor, fogFactor);

    return float4(finalColor, albedo.a);
}