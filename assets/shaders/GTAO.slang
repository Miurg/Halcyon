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

float3 reconstructViewPos(float2 uv, float depth)
{
	float4x4 proj = camera[0].projMatrix;
	float viewZ = proj[3][2] / (-depth - proj[2][2]);
	viewZ = -abs(viewZ);
	float negViewZ = -viewZ;

	float2 ndc = uv * 2.0 - 1.0;
	float3 viewPos;
	viewPos.x = ndc.x * negViewZ / proj[0][0];
	viewPos.y = ndc.y * negViewZ / proj[1][1];
	viewPos.z = viewZ;
	return viewPos;
}

static const float PI = 3.14159265;
static const float HALF_PI = 1.5707963;

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target
{
	float depth = depthTexture.SampleLevel(input.uv, 0).r;
	if (depth <= 0.0) return float4(1.0, 1.0, 1.0, 1.0);

	float3 P = reconstructViewPos(input.uv, depth);

	float fade = smoothstep(-pc.fadeEnd, -pc.fadeStart, P.z);
	if (fade <= 0.001) return float4(1.0, 1.0, 1.0, 1.0);

	float3 N = normalize(normalsTexture.SampleLevel(input.uv, 0).xyz);

	float4x4 proj = camera[0].projMatrix;
	float screenRadius = (pc.radius * proj[0][0]) / abs(P.z);
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
		float hCosForward = -1.0;
		float hCosBackward = -1.0;

		float2 sampleUVF = input.uv + stepVec * (jitter * 0.5 + 0.1);
		float2 sampleUVB = input.uv - stepVec * (jitter * 0.5 + 0.1);

		for (int s = 0; s < numSteps; s++)
		{
			if (all(saturate(sampleUVF) == sampleUVF))
			{
				float depthF = depthTexture.SampleLevel(sampleUVF, 0).r;
				if (depthF > 0.0)
				{
					float3 SF = reconstructViewPos(sampleUVF, depthF);
					float3 HF = SF - P;
					float distF = length(HF);
					if (distF > 0.001 && distF < pc.radius && abs(HF.z) < pc.radius * 1.5)
						hCosForward = max(hCosForward, dot(HF, V) / distF);
				}
			}

			if (all(saturate(sampleUVB) == sampleUVB))
			{
				float depthB = depthTexture.SampleLevel(sampleUVB, 0).r;
				if (depthB > 0.0)
				{
					float3 SB = reconstructViewPos(sampleUVB, depthB);
					float3 HB = SB - P;
					float distB = length(HB);
					if (distB > 0.001 && distB < pc.radius && abs(HB.z) < pc.radius * 1.5)
						hCosBackward = max(hCosBackward, dot(HB, V) / distB);
				}
			}

			sampleUVF += stepVec;
			sampleUVB -= stepVec;
		}

		float3 d_vs = normalize(float3(rotatedDir.x / proj[0][0], rotatedDir.y / proj[1][1], 0.0));
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
		float arc1 = -cos(2.0 * h1 - n) + cos(n) + 2.0 * h1 * sin(n);
		float arc2 = -cos(2.0 * h2 - n) + cos(n) + 2.0 * h2 * sin(n);
		visibility += projNLen * (arc1 + arc2) * 0.25;
	}

	visibility /= float(pc.numDirections);
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
