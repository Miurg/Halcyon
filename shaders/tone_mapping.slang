[vk::binding(0, 0)]
Sampler2D sourceTexture;

[vk::binding(1, 1)]
StructuredBuffer<float> exposureBuffer;

[[vk::constant_id(0)]]
const int ENABLE_AUTO_EXPOSURE = 1;

struct ColorGrading
{
	int space;         // 0 = Display (post-tonemap), 1 = Linear (HDR), 2 = Log (pre-tonemap)
	int toneMapper;
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
static const int TONE_MAPPER_AGX = 0;
static const int TONE_MAPPER_ACES_FILMIC = 1;
static const int TONE_MAPPER_GT7 = 2;
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

// === AgX Tone Mapping ===
// Based on Troy Lim's AgX, GLSL reference by bwrensch
static const float3x3 AgXInputMatrix = float3x3(
	0.856627153315983, 0.0951212405381588, 0.0482516061458583,
	0.137318972929847, 0.761241990602591, 0.101439036467562,
	0.11189821299995, 0.0767994186031903, 0.811302368396859
);

static const float3x3 AgXOutputMatrix = float3x3(
	 1.1271005818144368, -0.11060664309660323, -0.016493938717834573,
	-0.1413297634984383, 1.157823702216272, -0.016493938717834257,
	-0.14132976349843826, -0.11060664309660294, 1.2519364065950405
);

float3 AgXLinearSrgbToRec2020(float3 color)
{
    return float3(
        dot(color, float3(0.627403896, 0.329283038, 0.043313066)),
        dot(color, float3(0.069097289, 0.919540395, 0.011362316)),
        dot(color, float3(0.016391439, 0.088013308, 0.895595253)));
}

float3 AgXLinearRec2020ToSrgb(float3 color)
{
    return float3(
        dot(color, float3( 1.660491002, -0.587641139, -0.072849863)),
        dot(color, float3(-0.124550475,  1.132899897, -0.008349423)),
        dot(color, float3(-0.018150763, -0.100578898,  1.118729661)));
}

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
    color = AgXLinearSrgbToRec2020(max(color, 0.0));

    color = mul(AgXInputMatrix, color);

    const float minEV = -12.47393;
    const float maxEV = 4.026069;
    color = log2(max(color, 1e-10));
    color = saturate((color - minEV) / (maxEV - minEV));

    color = AgXDefaultContrastApprox(color);

    color = mul(AgXOutputMatrix, color);
    color = pow(max(color, 0.0), 2.2);

    return saturate(AgXLinearRec2020ToSrgb(color));
}

/*
GT7 Tone Mapping SDR/ICtCp port from Polyphony Digital's official sample implementation v1.0.
The surrounding gamut transforms are needed because the operator expects linear Rec.2020 while this renderer uses
linear sRGB.

MIT License

Copyright (c) 2025 Polyphony Digital Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
float3 LinearSrgbToRec2020(float3 color)
{
    return float3(
        dot(color, float3(0.627403896, 0.329283038, 0.043313066)),
        dot(color, float3(0.069097289, 0.919540395, 0.011362316)),
        dot(color, float3(0.016391439, 0.088013308, 0.895595253)));
}

float3 LinearRec2020ToSrgb(float3 color)
{
    return float3(
        dot(color, float3( 1.660491002, -0.587641139, -0.072849863)),
        dot(color, float3(-0.124550475,  1.132899897, -0.008349423)),
        dot(color, float3(-0.018150763, -0.100578898,  1.118729661)));
}

float GT7Curve(float x)
{
    const float peakIntensity = 2.5;
    const float alpha = 0.25;
    const float midPoint = 0.538;
    const float linearSection = 0.444;
    const float toeStrength = 1.280;
    const float k = (linearSection - 1.0) / (alpha - 1.0);
    const float kA = peakIntensity * linearSection + peakIntensity * k;
    const float kB = -peakIntensity * k * exp(linearSection / k);
    const float kC = -1.0 / (k * peakIntensity);

    x = max(x, 0.0);
    float weightLinear = smoothstep(0.0, midPoint, x);
    float weightToe = 1.0 - weightLinear;
    float shoulder = kA + kB * exp(x * kC);

    if (x < linearSection * peakIntensity)
    {
        float toeMapped = midPoint * pow(x / midPoint, toeStrength);
        return weightToe * toeMapped + weightLinear * x;
    }

    return shoulder;
}

float GT7EotfSt2084(float n)
{
    const float m1 = 0.1593017578125;
    const float m2 = 78.84375;
    const float c1 = 0.8359375;
    const float c2 = 18.8515625;
    const float c3 = 18.6875;

    float np = pow(saturate(n), 1.0 / m2);
    float luminance = max(np - c1, 0.0) / (c2 - c3 * np);
    return pow(luminance, 1.0 / m1) * 100.0;
}

float GT7InverseEotfSt2084(float value)
{
    const float m1 = 0.1593017578125;
    const float m2 = 78.84375;
    const float c1 = 0.8359375;
    const float c2 = 18.8515625;
    const float c3 = 18.6875;

    float y = max(value, 0.0) * 0.01;
    float ym = pow(y, m1);
    return exp2(m2 * (log2(c1 + c2 * ym) - log2(1.0 + c3 * ym)));
}

float3 GT7RgbToICtCp(float3 color)
{
    float3 lms = float3(
        dot(color, float3(1688.0, 2146.0, 262.0)),
        dot(color, float3(683.0, 2951.0, 462.0)),
        dot(color, float3(99.0, 309.0, 3688.0))) / 4096.0;
    float3 lmsPq = float3(
        GT7InverseEotfSt2084(lms.x),
        GT7InverseEotfSt2084(lms.y),
        GT7InverseEotfSt2084(lms.z));

    return float3(
        dot(lmsPq, float3(2048.0, 2048.0, 0.0)),
        dot(lmsPq, float3(6610.0, -13613.0, 7003.0)),
        dot(lmsPq, float3(17933.0, -17390.0, -543.0))) / 4096.0;
}

float3 GT7ICtCpToRgb(float3 ictCp)
{
    float3 lmsPq = float3(
        ictCp.x + 0.00860904 * ictCp.y + 0.11103 * ictCp.z,
        ictCp.x - 0.00860904 * ictCp.y - 0.11103 * ictCp.z,
        ictCp.x + 0.560031 * ictCp.y - 0.320627 * ictCp.z);
    float3 lms = float3(
        GT7EotfSt2084(lmsPq.x),
        GT7EotfSt2084(lmsPq.y),
        GT7EotfSt2084(lmsPq.z));

    return max(float3(
        dot(lms, float3(3.43661, -2.50645, 0.0698454)),
        dot(lms, float3(-0.79133, 1.9836, -0.192271)),
        dot(lms, float3(-0.0259499, -0.0989137, 1.12486))), 0.0);
}

float3 GT7(float3 color)
{
    const float framebufferLuminanceTarget = 2.5;
    const float framebufferLuminanceTargetUcs = 0.602559155;
    const float blendRatio = 0.6;
    const float fadeStart = 0.98;
    const float fadeEnd = 1.16;
    const float sdrCorrectionFactor = 0.4;

    float3 rec2020 = LinearSrgbToRec2020(max(color, 0.0));
    float3 ucs = GT7RgbToICtCp(rec2020);
    float3 skewedRgb = float3(GT7Curve(rec2020.x), GT7Curve(rec2020.y), GT7Curve(rec2020.z));
    float3 skewedUcs = GT7RgbToICtCp(skewedRgb);
    float chromaScale = 1.0 - smoothstep(fadeStart, fadeEnd, ucs.x / framebufferLuminanceTargetUcs);
    float3 scaledUcs = float3(skewedUcs.x, ucs.yz * chromaScale);
    float3 scaledRgb = GT7ICtCpToRgb(scaledUcs);
    float3 mapped = sdrCorrectionFactor * min(lerp(skewedRgb, scaledRgb, blendRatio), framebufferLuminanceTarget);
    return max(LinearRec2020ToSrgb(mapped), 0.0);
}

float3 ApplyToneMapper(float3 color)
{
    if (grading.toneMapper == TONE_MAPPER_ACES_FILMIC)
    {
        return ACESFilm(color);
    }
    if (grading.toneMapper == TONE_MAPPER_GT7)
    {
        return GT7(color);
    }
    return AgX(color);
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
        mapped = ApplyToneMapper(c);
    }
    else if (grading.space == GRADE_LOG)
    {
        // Grade in log2 space (perceptually even, no highlight blow-out), then tone map.
        float3 logc = log2(max(hdr.rgb, 1e-5));
        logc = Grade(logc, log2(MID_GREY));
        mapped = ApplyToneMapper(exp2(logc));
    }
    else // GRADE_DISPLAY
    {
        // Grade after tone mapping, in display-referred space.
        mapped = Grade(ApplyToneMapper(hdr.rgb), 0.5);
    }

	return float4(saturate(mapped), 1.0);
}
