[vk::binding(0, 0)] Sampler2D depthTexture;
[vk::binding(1, 0)] Sampler2D normalsTexture;

struct CameraData
{
    float4x4 cameraSpaceMatrix;
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float4x4 invViewProj;
    float4   cameraPositionAndPadding;
    float4   frustumPlanes[6];
};
[[vk::binding(0, 1)]] StructuredBuffer<CameraData> camera;

struct PushConstants
{
    int   kernelSize;
    float radius;
    float bias;
    float power;
    int   numDirections;
    float maxScreenRadius;
};
[[vk::push_constant]] ConstantBuffer<PushConstants> pc;

struct VSOutput
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD0;
};

[shader("vertex")]
VSOutput vertMain(uint vertexID : SV_VertexID)
{
    VSOutput output;
    output.uv  = float2((vertexID << 1) & 2, vertexID & 2);
    output.pos = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    return output;
}

float3 reconstructViewPos(float2 uv, float depth)
{
    float4x4 proj = camera[0].projMatrix;
    float viewZ = proj[3][2] / (-depth - proj[2][2]);
    float negViewZ = -viewZ;
    
    float2 ndc = uv * 2.0 - 1.0;
    float3 viewPos;
    viewPos.x = ndc.x * negViewZ / proj[0][0]; 
    viewPos.y = ndc.y * negViewZ / proj[1][1];
    viewPos.z = viewZ; 
    return viewPos;
}

float falloff(float distSq, float RrSq)
{
    float factor = distSq * RrSq;
    return saturate(1.0 - factor * factor);
}

float GetIGN(float2 pixelPos)
{
    float3 magic = float3(0.06711056f, 0.00583715f, 52.9829189f);
    return frac(magic.z * frac(dot(pixelPos, magic.xy)));
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target
{
    float depth = depthTexture.SampleLevel(input.uv, 0).r;

    if (depth >= 1.0) 
        return float4(1.0, 1.0, 1.0, 1.0);

    float3 P = reconstructViewPos(input.uv, depth);
    float3 N = normalize(normalsTexture.SampleLevel(input.uv, 0).xyz);

    float4x4 proj = camera[0].projMatrix;
    float screenRadius = (pc.radius * proj[0][0]) / abs(P.z);
    screenRadius = min(screenRadius, pc.maxScreenRadius); 
    
    if(screenRadius < 0.001) return float4(1.0, 1.0, 1.0, 1.0);

    float RrSq = 1.0 / (pc.radius * pc.radius);

    float ign = GetIGN(input.pos.xy);
    
    float randomAngle = ign * 6.28318530718;
    float2 randDir = float2(cos(randomAngle), sin(randomAngle));
    
    float jitter = ign;

    int NUM_DIRECTIONS = pc.numDirections;
    int numSteps = pc.kernelSize;
    
    float stepUV = screenRadius / (float(numSteps) + 1.0);
    
    float occlusion = 0.0;
    float directionStepAngle = (2.0 * 3.14159265) / float(NUM_DIRECTIONS);
    
    for (int d = 0; d < NUM_DIRECTIONS; d++)
    {
        float angle = float(d) * directionStepAngle;
        float2 baseDir = float2(cos(angle), sin(angle));
        
        float2 rotatedDir;
        rotatedDir.x = baseDir.x * randDir.x - baseDir.y * randDir.y;
        rotatedDir.y = baseDir.x * randDir.y + baseDir.y * randDir.x;
        
        float2 stepVec = rotatedDir * stepUV;
        float2 sampleUV = input.uv + stepVec * (jitter + 0.1);
        
        float topOcclusion = 0.0;
        
        for (int s = 0; s < numSteps; s++)
        {
            if(any(saturate(sampleUV) != sampleUV)) break;

            float depthS = depthTexture.SampleLevel(sampleUV, 0).r;
            
            if (depthS < 1.0)
            {
                float3 S = reconstructViewPos(sampleUV, depthS);
                float3 V = S - P;
                float VdotV = dot(V, V);

                bool isNotTooFarBack = (S.z - P.z) > -(pc.radius * 0.8);

                if (VdotV < (pc.radius * pc.radius) && isNotTooFarBack)
                {
                    float NdotV = dot(N, normalize(V));
                    float attenuation = falloff(VdotV, RrSq);
                    float currentOcclusion = saturate(NdotV - pc.bias) * attenuation;
                    topOcclusion = max(topOcclusion, currentOcclusion);
                }
            }
            
            sampleUV += stepVec;
        }
        
        occlusion += topOcclusion;
    }
    
    occlusion /= float(NUM_DIRECTIONS);
    occlusion = pow(1.0 - saturate(occlusion), pc.power);
    
    return float4(occlusion, occlusion, occlusion, 1.0);
}