[vk::binding(0, 0)]
Sampler2D depthTexture;
[vk::binding(1, 0)]
Sampler2D normalsTexture;
[vk::binding(2, 0)]
Sampler2D noiseTexture;

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
	int kernelSize;
	float radius;
	float bias;
	float power;
	int numDirections;
	float maxScreenRadius;
	float fadeStart;
	float fadeEnd;
	float2 texelSize;
};
[[vk::push_constant]]
ConstantBuffer<PushConstants> pc;

struct VSOutput
{
	float4 pos : SV_Position;
	float2 uv : TEXCOORD0;
};

[shader("vertex")]
VSOutput vertMain(uint vertexID: SV_VertexID)
{
	VSOutput output;
	output.uv = float2((vertexID << 1) & 2, vertexID & 2);
	output.pos = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
	return output;
}

float3 reconstructViewPos(float2 uv, float depth, float invP00, float invP11, float p22, float p32)
{
	float viewZ = p32 / (-depth - p22);
	viewZ = -abs(viewZ);
	float negViewZ = -viewZ;
	float2 ndc = uv * 2.0 - 1.0;
	return float3(ndc.x * negViewZ * invP00, ndc.y * negViewZ * invP11, viewZ);
}

static const float PI = 3.14159265;
static const float HALF_PI = 1.5707963;

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target
{
	float depth = depthTexture.SampleLevel(input.uv, 0).r;
	if (depth <= 0.0) return float4(1.0, 1.0, 1.0, 1.0);

	// Cache all needed projection scalars once — reconstructViewPos is called per-sample.
	float4x4 proj = camera[0].projMatrix;
	float p00 = proj[0][0], p11 = proj[1][1];
	float invP00 = 1.0 / p00, invP11 = 1.0 / p11;
	float p22 = proj[2][2], p32 = proj[3][2];

	float3 P = reconstructViewPos(input.uv, depth, invP00, invP11, p22, p32);

	float fade = smoothstep(-pc.fadeEnd, -pc.fadeStart, P.z);
	if (fade <= 0.001) return float4(1.0, 1.0, 1.0, 1.0);

	float3 N = normalize(normalsTexture.SampleLevel(input.uv, 0).xyz);

	float screenRadius = (pc.radius * p00) / abs(P.z);
	screenRadius = min(screenRadius, pc.maxScreenRadius);
	if (screenRadius < 0.001) return float4(1.0, 1.0, 1.0, 1.0);

	float3 V = normalize(-P);

	float2 noiseUV = input.pos.xy / 64.0;
	float2 blueNoise = noiseTexture.SampleLevel(noiseUV, 0).rg;
	float randomAngle = blueNoise.r * 6.28318530718;
	float2 randDir = float2(cos(randomAngle), sin(randomAngle));
	float jitter = blueNoise.g;

	int numSteps = pc.kernelSize;
	float stepUV = screenRadius / (float(numSteps) + 1.0);
	stepUV = max(stepUV, length(pc.texelSize));

	float visibility = 0.0;
	// PI not 2*PI: each slice already covers both +d and -d.
	float dirAngle = PI / float(pc.numDirections);
	float invNumDirs = 1.0 / float(pc.numDirections);

	float radius2 = pc.radius * pc.radius;
	float thicknessLimit = pc.radius * 1.5;
	float jitterOffset = jitter * 0.5 + 0.1;

	for (int d = 0; d < pc.numDirections; d++)
	{
		float angle = float(d) * dirAngle;
		float2 baseDir = float2(cos(angle), sin(angle));

		float2 rotatedDir;
		rotatedDir.x = baseDir.x * randDir.x - baseDir.y * randDir.y;
		rotatedDir.y = baseDir.x * randDir.y + baseDir.y * randDir.x;

		float2 stepVec = rotatedDir * stepUV;

		// Init to -1 so absence of occluder yields visibility >= 1 (saturated to 1 later);
		// init at 0 would clip the arc at the view tangent and darken tilted flat surfaces.
		float hCosForward  = -1.0;
		float hCosBackward = -1.0;

		float2 sampleUVF = input.uv + stepVec * jitterOffset;
		float2 sampleUVB = input.uv - stepVec * jitterOffset;

		for (int s = 0; s < numSteps; s++)
		{
			float depthF = depthTexture.SampleLevel(sampleUVF, 0).r;
			if (depthF > 0.0)
			{
				float3 SF = reconstructViewPos(sampleUVF, depthF, invP00, invP11, p22, p32);
				float3 HF = SF - P;
				float dist2F = dot(HF, HF);
				if (dist2F > 1e-6 && dist2F < radius2 && abs(HF.z) < thicknessLimit)
					hCosForward = max(hCosForward, dot(HF, V) * rsqrt(dist2F));
			}

			float depthB = depthTexture.SampleLevel(sampleUVB, 0).r;
			if (depthB > 0.0)
			{
				float3 SB = reconstructViewPos(sampleUVB, depthB, invP00, invP11, p22, p32);
				float3 HB = SB - P;
				float dist2B = dot(HB, HB);
				if (dist2B > 1e-6 && dist2B < radius2 && abs(HB.z) < thicknessLimit)
					hCosBackward = max(hCosBackward, dot(HB, V) * rsqrt(dist2B));
			}

			sampleUVF += stepVec;
			sampleUVB -= stepVec;
		}

		float3 d_vs = normalize(float3(rotatedDir.x * invP00, rotatedDir.y * invP11, 0.0));
		float3 sliceN = cross(V, d_vs);
		float sliceNLen = length(sliceN);
		if (sliceNLen < 0.001) continue;
		sliceN /= sliceNLen;
		float3 orthoDir = cross(sliceN, V);

		float3 projN = N - sliceN * dot(N, sliceN);
		float projNLen = length(projN);
		if (projNLen < 0.001) continue;

		float cosN = saturate(dot(projN, V) / projNLen);
		float signN = sign(dot(orthoDir, projN));
		float n = signN * acos(cosN);

		// h2 on +orthoDir side (positive), h1 on -orthoDir side (negative).
		float h2 = acos(clamp(hCosForward, -1.0, 1.0));
		float h1 = -acos(clamp(hCosBackward, -1.0, 1.0));

		// Dead zone around V to avoid self-occlusion on near-tangent samples.
		h2 = max(h2, pc.bias);
		h1 = min(h1, -pc.bias);

		h1 = max(h1, n - HALF_PI);
		h2 = min(h2, n + HALF_PI);

		// Jimenez 2016, eq. 10.
		// cos(signN * acos(cosN)) == cosN by definition; sin derived via Pythagorean identity —
		// avoids two extra transcendental calls per direction.
		float sinN = signN * sqrt(max(0.0, 1.0 - cosN * cosN));
		float arc1 = -cos(2.0 * h1 - n) + cosN + 2.0 * h1 * sinN;
		float arc2 = -cos(2.0 * h2 - n) + cosN + 2.0 * h2 * sinN;
		visibility += projNLen * (arc1 + arc2) * 0.25;
	}

	visibility *= invNumDirs;
	// Saturate before pow: tilted flat surfaces produce arc-integral > 1; without clipping
	// pow would only kick in for real occluders, leaving flat surfaces brighter than 1.
	visibility = saturate(visibility);
	visibility = pow(visibility, pc.power);
	visibility = lerp(1.0, visibility, fade);

	return float4(visibility, visibility, visibility, 1.0);
}
// ========================================================================================
// Method: GTAO (Ground Truth Ambient Occlusion)
// Authors: Jorge Jimenez, Xian-Chun Wu, Tobias Ritschel, Jan Kautz
// Paper: "Practical Realtime Strategies for Accurate Indirect Occlusion", SIGGRAPH 2016
// ========================================================================================
