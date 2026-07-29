#include "Shared/Bindings.h"

import Common.BRDF;
import Common.IBL;
import Common.Constants;

[[vk::binding(BIND_TEXTURES_CUBEMAP_STORAGE, 0)]]
[format("rg16f")]
RWTexture2D<float2> outBrdfLut;

float2 IntegrateBRDF(float NdotV, float roughness)
{
    float3 V;
    V.x = sqrt(1.0 - NdotV * NdotV);
    V.y = 0.0;
    V.z = NdotV;

    float A = 0.0;
    float B = 0.0;
    float3 N = float3(0.0, 0.0, 1.0);

    for (uint i = 0u; i < SAMPLE_COUNT; i++)
    {
        float2 Xi = Hammersley(i, SAMPLE_COUNT);
        float3 H = ImportanceSampleGGX(Xi, N, roughness);
        float3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if (NdotL > 0.0)
        {
            float G = GeometrySmith(NdotV, NdotL, roughness);
            float G_Vis = (G * VdotH) / max(NdotH * NdotV, 1e-4);
            float Fc = pow(1.0 - VdotH, 5.0);

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }

    return saturate(float2(A, B) / float(SAMPLE_COUNT));
}

[shader("compute")]
[numthreads(8, 8, 1)]
void computeMain(uint3 threadId: SV_DispatchThreadID)
{
	uint width, height;
	outBrdfLut.GetDimensions(width, height);

	if (threadId.x >= width || threadId.y >= height) return;

	float NdotV = (float(threadId.x) + 0.5) / float(width);
	float roughness = (float(threadId.y) + 0.5) / float(height);

	// Clamp NdotV to avoid division by zero
	NdotV = max(NdotV, 0.001);
	roughness = max(roughness, 0.001);

    float2 result = IntegrateBRDF(NdotV, roughness);
    outBrdfLut[threadId.xy] = result;
}
