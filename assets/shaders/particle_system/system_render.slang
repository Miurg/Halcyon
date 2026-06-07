// === Set 0: particleEmiterSet ===
struct Particle
{
	float3 position;
	float _padding;
	float3 scale;
	float timeToDie;
};

[vk::binding(0, 0)]
StructuredBuffer<Particle> particlesBuffer;

[vk::binding(5, 0)]
StructuredBuffer<uint> aliveIndicesBuffer;

struct CameraData
{
    float4x4 cameraSpaceMatrix;
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float4x4 invViewProj;
    float4 cameraPositionAndPadding;
    float4 frustumPlanes[6];
};
[[vk::binding(0, 1)]]
StructuredBuffer<CameraData> camera;

struct VSOutput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
};

static const float2 quadPositions[6] = {
    float2(-0.5, -0.5),
    float2(0.5, -0.5),
    float2(-0.5, 0.5),
    float2(-0.5, 0.5),
    float2(0.5, -0.5),
    float2(0.5, 0.5)
};

[shader("vertex")]
VSOutput vertMain(uint vertexID: SV_VertexID, uint instanceID: SV_InstanceID, uint baseInstance: SV_StartInstanceLocation)
{
    uint realIndex = aliveIndicesBuffer[baseInstance + instanceID];
    Particle particle = particlesBuffer[realIndex];
   
    float2 localPos = quadPositions[vertexID];
    float2 scaledLocalPos = localPos * particle.scale.xy;

    float4 viewPos = mul(camera[0].viewMatrix, float4(particle.position, 1.0));
    viewPos.xy += scaledLocalPos;

    VSOutput output;
    output.pos = mul(camera[0].projMatrix, viewPos);
    output.uv = localPos + 0.5;

    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target
{
    return float4(1.0, 1.0, 1.0, 1.0);
}