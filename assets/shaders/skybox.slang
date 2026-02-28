struct CameraData
{
    float4x4 cameraSpaceMatrix;
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float4x4 invViewProj;
    float4   cameraPositionAndPadding;
    float4   frustumPlanes[6];
};
[[vk::binding(0, 0)]] StructuredBuffer<CameraData> camera;

[[vk::binding(0, 2)]] Sampler2D        textureArray[2048];
[[vk::binding(3, 2)]] SamplerCube      skyboxMap;

struct VSOutput
{
    float4 pos          : SV_Position;
    float3 localPos;
};

[shader("vertex")]
VSOutput vertMain(uint vertexID : SV_VertexID)
{
    // Generate a fullscreen triangle on the far plane (z = 1.0)
    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
    float4 pos = float4(uv * 2.0 - 1.0, 1.0, 1.0);
    
    VSOutput output;
    output.pos = pos;
    // We map clip space to world space to get ray direction for sampling the skybox
    float4 target = mul(camera[0].invViewProj, pos);
    output.localPos = target.xyz / target.w - camera[0].cameraPositionAndPadding.xyz;
    
    return output;
}

struct FragOutput
{
    float4 color    : SV_Target0;
    float4 normals  : SV_Target1;
};

[shader("fragment")]
FragOutput fragMain(VSOutput input)
{
    FragOutput output;
    
    // Sample skybox using the reconstructed local ray direction
    float3 envColor = skyboxMap.Sample(input.localPos).rgb;
    
    output.color = float4(envColor, 1.0);
    output.normals = float4(0.0, 0.0, 0.0, 0.0); // Don't disrupt normals target
    
    return output;
}
