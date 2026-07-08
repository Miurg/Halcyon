[vk::binding(0, 0)]
Sampler2D sourceTexture;

[vk::binding(1, 1)]
StructuredBuffer<float> exposureBuffer;

[[vk::constant_id(0)]]
const int ENABLE_AUTO_EXPOSURE = 1;

struct ColorGrading
{
	int space;         // 0 = Display (post-AgX), 1 = Linear (HDR), 2 = Log (pre-AgX)
	float exposure;    // EV compensation, applied in linear before tone mapping
	float contrast;    // 1.0 = neutral
	float saturation;  // 1.0 = neutral
	float temperature; // [-1, 1], negative = cooler, positive = warmer
	float tint;        // [-1, 1], green/magenta balance
};
[[vk::push_constant]]
ConstantBuffer<ColorGrading> grading;

static const int GRADE_DISPLAY = 0;
static const int GRADE_LINEAR = 1;
static const int GRADE_LOG = 2;
static const float MID_GREY = 0.18; // scene-referred middle grey

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

// TODO: Add Reinhard, Uncharted 2, Hable, etc. tone mapping operators for comparison.

// === Tone Mapping ===
// ACES Filmic (Narkowicz 2015)
float3 ACESFilm(float3 x)
{
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;
	return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

// All this stuff below doesn't work like it should. 
// Maybe I messed up the matrices, or maybe it's just not a good fit for real-time rendering.
// Keep for future. 

// === AgX Tone Mapping ===
// Based on Troy Lim's AgX, GLSL reference by bwrensch
static const float3x3 AgXInputMatrix = float3x3(
	0.856627153315983, 0.137318972929847, 0.11189821299995,
	0.0951212405381588, 0.761241990602591, 0.0767994186031903,
	0.0482516061458583, 0.101439036467562, 0.811302368396859
);

static const float3x3 AgXOutputMatrix = float3x3(
	 1.1271005818144368, -0.1413297634984383, -0.14132976349843826,
	-0.11060664309660323, 1.157823702216272, -0.11060664309660294,
	-0.01647326386032981, -0.016473263860329533, 1.2519364065950405
);
float3 AgXDefaultContrastApprox(float3 x)
{
    float3 x2 = x * x;
    float3 x4 = x2 * x2;
    return +15.5 * x4 * x2
	       - 40.14 * x4 * x
	       + 31.96 * x4
	       - 6.868 * x2 * x
	       + 0.4298 * x2
	       + 0.1191 * x
	       - 0.00232;
}

float3 AgX(float3 color)
{
    color = max(color, 0.0);

    color = mul(AgXInputMatrix, color);

    const float minEV = -10.0;
    const float maxEV = 6.5;
    color = clamp(log2(color), minEV, maxEV);
    color = (color - minEV) / (maxEV - minEV); 

    color = AgXDefaultContrastApprox(color);

    color = mul(AgXOutputMatrix, color);

    return color;
}

// === Color Grading ===
// White balance via LMS chromatic adaptation. Reference: Unity Post Processing Stack v2.
float3 WhiteBalance(float3 color, float temperature, float tint)
{
    float t1 = temperature * 10.0 / 6.0;
    float t2 = tint * 10.0 / 6.0;

    float x = 0.31271 - t1 * (t1 < 0.0 ? 0.1 : 0.05);
    float standardIlluminantY = 2.87 * x - 3.0 * x * x - 0.27509507;
    float y = standardIlluminantY + t2 * 0.05;

    float3 w1 = float3(0.949237, 1.03542, 1.08728); // D65 white point in LMS

    float Y = 1.0;
    float X = Y * x / y;
    float Z = Y * (1.0 - x - y) / y;
    float L =  0.7328 * X + 0.4296 * Y - 0.1624 * Z;
    float M = -0.7036 * X + 1.6975 * Y + 0.0061 * Z;
    float S =  0.0030 * X + 0.0136 * Y + 0.9834 * Z;
    float3 w2 = float3(L, M, S);

    float3 balance = w1 / w2;

    static const float3x3 LIN_2_LMS = float3x3(
        3.90405e-1, 5.49941e-1, 8.92632e-3,
        7.08416e-2, 9.63172e-1, 1.35775e-3,
        2.31082e-2, 1.28021e-1, 9.36245e-1);
    static const float3x3 LMS_2_LIN = float3x3(
         2.85847e+0, -1.62879e+0, -2.48910e-2,
        -2.10182e-1,  1.15820e+0,  3.24281e-4,
        -4.18120e-2, -1.18169e-1,  1.06867e+0);

    float3 lms = mul(LIN_2_LMS, color);
    lms *= balance;
    return mul(LMS_2_LIN, lms);
}

float3 ApplyContrast(float3 color, float contrast, float pivot)
{
    return (color - pivot) * contrast + pivot;
}

float3 ApplySaturation(float3 color, float saturation)
{
    float luma = dot(color, float3(0.2126, 0.7152, 0.0722));
    return lerp(luma.xxx, color, saturation);
}

// Saturation + contrast applied around a space-specific pivot.
float3 Grade(float3 color, float pivot)
{
    color = ApplySaturation(color, grading.saturation);
    color = ApplyContrast(color, grading.contrast, pivot);
    return color;
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target
{
    float4 hdr = sourceTexture.SampleLevel(input.uv, 0.0);
    if (ENABLE_AUTO_EXPOSURE != 0)
    {
        hdr.rgb *= exposureBuffer[0];
    }

    hdr.rgb *= exp2(grading.exposure);
    hdr.rgb = max(WhiteBalance(hdr.rgb, grading.temperature, grading.tint), 0.0);

    float3 mapped;
    if (grading.space == GRADE_LINEAR)
    {
        // Grade in scene-referred linear light, then tone map.
        float3 c = max(Grade(hdr.rgb, MID_GREY), 0.0);
        mapped = AgX(c);
    }
    else if (grading.space == GRADE_LOG)
    {
        // Grade in log2 space (perceptually even, no highlight blow-out), then tone map.
        float3 logc = log2(max(hdr.rgb, 1e-5));
        logc = Grade(logc, log2(MID_GREY));
        mapped = AgX(exp2(logc));
    }
    else // GRADE_DISPLAY
    {
        // Grade after tone mapping, in display-referred space.
        mapped = Grade(AgX(hdr.rgb), 0.5);
    }

	return float4(saturate(mapped), 1.0);
}
