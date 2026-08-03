#include "Shared/GpuStructs.h"
#include "Shared/Bindings.h"

import Common.Constants;
import Common.Shadow;

[vk::binding(0, 0)]
Sampler2D sourceTexture;

[vk::binding(1, 0)]
Texture2D<float> depth;

[[vk::binding(BIND_GLOBAL_CAMERA, 1)]]
StructuredBuffer<CameraData> camera;

[vk::binding(BIND_GLOBAL_SUN, 1)]
StructuredBuffer<DirectionalLightData> directionalLight;

[[vk::binding(BIND_TEXTURES_SHADOW_MAP, 2)]]
Sampler2DShadow shadowMap;

struct GodRaysPushConstants
{
    uint raymarchStepCount;
    float extinctionCoefficient;
    float scatteringCoefficient;
    float fogDensity;
};
[[vk::push_constant]]
GodRaysPushConstants godRaysSettings;

struct VSOutput
{
	float4 pos : SV_Position;
	float2 uv : TEXCOORD0;
	noperspective float4 worldNearH : TEXCOORD1;
	noperspective float4 worldFarH : TEXCOORD2;
};

[shader("vertex")]
VSOutput vertMain(uint vertexID: SV_VertexID)
{
	VSOutput output;
	output.uv = float2((vertexID << 1) & 2, vertexID & 2);
	output.pos = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);

	float2 ndc = output.pos.xy;

	float4 clipZ0 = float4(ndc, 0.0, 1.0);
	float4 clipZ1 = float4(ndc, 1.0, 1.0);

	output.worldFarH = mul(camera[0].invViewProj, float4(ndc, 0.0, 1.0));

	output.worldNearH = mul(camera[0].invViewProj, float4(ndc, 1.0, 1.0));

	return output;
}

float SampleFogDensity(float3 position)
{
	return godRaysSettings.fogDensity;
}

float PhaseFunction(float cosTheta)
{
	return 1.0 / (4.0 * PI);
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target
{
	float4 source = sourceTexture.Load(int3(input.pos.xy, 0));
	float depthValue = depth.Load(int3(uint2(input.pos.xy), 0));

	if (depthValue == 0.0f)
	{
		float3 defaultScattering = directionalLight[0].color.rgb * PhaseFunction(0.0f) * (godRaysSettings.scatteringCoefficient / godRaysSettings.extinctionCoefficient);

		return float4(source.rgb + defaultScattering, source.a);
	}

	float4 surfaceH = lerp(input.worldFarH, input.worldNearH, depthValue);

	float3 wolrdSpaceNear = input.worldNearH.xyz / input.worldNearH.w;
	float3 worldSpaceSurface = surfaceH.xyz * rcp(surfaceH.w);

	float3 ray = worldSpaceSurface - wolrdSpaceNear;
	float lengthOfOneStep = length(ray) / godRaysSettings.raymarchStepCount;
	float3 stepVector = ray / godRaysSettings.raymarchStepCount;

	float3 stepWolrdPos = wolrdSpaceNear;

	float4 shadowCoord = mul(directionalLight[0].lightSpaceMatrix, float4(wolrdSpaceNear, 1.0));
	float4 shadowCoordStep = mul(directionalLight[0].lightSpaceMatrix, float4(stepVector, 0.0));

	float accumulatedLight = 0.0;
	float transmittance = 1.0;

	for (uint i = 0; i < godRaysSettings.raymarchStepCount; ++i)
	{
		float density = SampleFogDensity(stepWolrdPos);
		float extinction = density * godRaysSettings.extinctionCoefficient;
		float visibility = ComputeShadowPCF(shadowMap, shadowCoord, directionalLight[0].shadowMapSize.zw, 0.0001);

		float scattering = visibility * PhaseFunction(0.0f) * density * godRaysSettings.scatteringCoefficient;

		accumulatedLight += transmittance * scattering * lengthOfOneStep;
		transmittance *= exp(-extinction * lengthOfOneStep);
		stepWolrdPos += stepVector;
		shadowCoord += shadowCoordStep;
	}

    float3 finalColor = source.rgb * transmittance + (accumulatedLight * directionalLight[0].color.rgb * directionalLight[0].color.a);
	return float4(finalColor, source.a);
}
