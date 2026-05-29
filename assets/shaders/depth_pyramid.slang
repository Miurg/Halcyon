[[vk::binding(0, 0)]]
Sampler2D<float> srcTex;

[[vk::binding(1, 0)]]
[format("r32f")]
RWTexture2D<float> dstMip;

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

struct PushConstants
{
	uint dstWidth;
	uint dstHeight;
	uint passIdx;
	float edgeRange;  // width of the weighted average around the farthest sample
};
[[vk::push_constant]]
PushConstants push;

[shader("compute")]
[numthreads(8, 8, 1)]
void computeMain(uint3 id: SV_DispatchThreadID)
{
	if (id.x >= push.dstWidth || id.y >= push.dstHeight) return;

	// Linearize hardware (reverse-Z) depth into positive view-space Z
	if (push.passIdx == 0)
	{
		float2 uv = (id.xy + 0.5) / float2(push.dstWidth, push.dstHeight);
		float d = srcTex.SampleLevel(uv, 0);
		float4x4 proj = camera[0].projMatrix;
		float p22 = proj[2][2];
		float p32 = proj[3][2];
		// Background (d <= 0) maps to the far-plane linear Z
		dstMip[id.xy] = abs(p32 / (-d - p22));
		return;
	}

	// Mip > 0: edge-aware weighted reduction of the previous (linear-Z) mip.
	uint2 srcSize;
	srcTex.GetDimensions(srcSize.x, srcSize.y);
	uint2 base = id.xy * 2;

	float depths[9];
	int n = 0;
	depths[n++] = srcTex.Load(int3(base + uint2(0, 0), 0));
	depths[n++] = srcTex.Load(int3(base + uint2(1, 0), 0));
	depths[n++] = srcTex.Load(int3(base + uint2(0, 1), 0));
	depths[n++] = srcTex.Load(int3(base + uint2(1, 1), 0));

	if (srcSize.x & 1)
	{
		depths[n++] = srcTex.Load(int3(base + uint2(2, 0), 0));
		depths[n++] = srcTex.Load(int3(base + uint2(2, 1), 0));
	}
	if (srcSize.y & 1)
	{
		depths[n++] = srcTex.Load(int3(base + uint2(0, 2), 0));
		depths[n++] = srcTex.Load(int3(base + uint2(1, 2), 0));
	}
	if ((srcSize.x & 1) && (srcSize.y & 1))
	{
		depths[n++] = srcTex.Load(int3(base + uint2(2, 2), 0));
	}

	// XeGTAO-style weighted average biased toward the farther sample
	float maxD = depths[0];
	for (int i = 1; i < n; i++) maxD = max(maxD, depths[i]);

	float range = maxD * push.edgeRange + 1e-4;
	float wSum = 0.0;
	float dSum = 0.0;
	for (int j = 0; j < n; j++)
	{
		float w = saturate(1.0 - (maxD - depths[j]) / range);
		wSum += w;
		dSum += depths[j] * w;
	}

	dstMip[id.xy] = dSum / max(wSum, 1e-5);
}
