struct FxaaConfig
{
    // The amount of sub-pixel aliasing removal.
    // 1.00 - upper limit (softer)
    // 0.75 - default amount of filtering
    // 0.50 - lower limit (sharper, less sub-pixel aliasing removal)
    // 0.25 - almost off
    // 0.00 - completely off
    float subpix;

    // The minimum amount of local contrast required to apply algorithm.
    // 0.333 - too little (faster)
    // 0.250 - low quality
    // 0.166 - default
    // 0.125 - high quality 
    // 0.063 - overkill (slower)
    float edgeThreshold;

    // Trims the algorithm from processing darks.
    // 0.0833 - upper limit (default, the start of visible darker colors)
    // 0.0625 - high quality (faster)
    // 0.0312 - visible limit (slower)
    float edgeThresholdMin;
};

static const FxaaConfig DefaultFxaaConfig = { 0.75, 0.166, 0.0833 };

// Helper to calculate Luma (perceptual brightness)
// FXAA works best if luma is pre-calculated in the Alpha channel of the input texture.
// If not, we calculate it on the fly using Rec. 601.
float FxaaLuma(float4 rgba)
{
    // return rgba.w; // Use this if Luma is stored in Alpha
    return dot(rgba.rgb, float3(0.299, 0.587, 0.114));
}

// ----------------------------------------------------------------------------------
// Main FXAA Function
// ----------------------------------------------------------------------------------
float4 FxaaPixelShader(
    float2 pos,      // Normalized UV coordinates of the current pixel
                       float2 rcpFrame, // 1.0 / ScreenResolution (Pixel size)
                       Sampler2D tex, // Linear Clamp Sampler
    FxaaConfig config = DefaultFxaaConfig
)
{
    float2 posM;
    posM.x = pos.x;
    posM.y = pos.y;
    
    // 1. Local Contrast Check & Luma Fetch
    // ------------------------------------
    float4 rgbyM = tex.SampleLevel(posM, 0.0);
    float lumaM = FxaaLuma(rgbyM);
    
    float lumaS = FxaaLuma(tex.SampleLevel(posM, 0.0, int2( 0, 1)));
    float lumaE = FxaaLuma(tex.SampleLevel(posM, 0.0, int2( 1, 0)));
    float lumaN = FxaaLuma(tex.SampleLevel(posM, 0.0, int2( 0,-1)));
    float lumaW = FxaaLuma(tex.SampleLevel(posM, 0.0, int2(-1, 0)));

    float maxSM = max(lumaS, lumaM);
    float minSM = min(lumaS, lumaM);
    float maxESM = max(lumaE, maxSM);
    float minESM = min(lumaE, minSM);
    float maxWN = max(lumaN, lumaW);
    float minWN = min(lumaN, lumaW);
    
    float rangeMax = max(maxWN, maxESM);
    float rangeMin = min(minWN, minESM);
    float rangeMaxScaled = rangeMax * config.edgeThreshold;
    float range = rangeMax - rangeMin;
    float rangeMaxClamped = max(config.edgeThresholdMin, rangeMaxScaled);
    
    // Early Exit: if contrast is lower than threshold, skip AA
    if (range < rangeMaxClamped) 
    {
        return rgbyM;
    }

    // 2. Sub-pixel Aliasing Test
    // ------------------------------------
    float lumaNW = FxaaLuma(tex.SampleLevel(posM, 0.0, int2(-1,-1)));
    float lumaSE = FxaaLuma(tex.SampleLevel(posM, 0.0, int2( 1, 1)));
    float lumaNE = FxaaLuma(tex.SampleLevel(posM, 0.0, int2( 1,-1)));
    float lumaSW = FxaaLuma(tex.SampleLevel(posM, 0.0, int2(-1, 1)));

    float lumaNS = lumaN + lumaS;
    float lumaWE = lumaW + lumaE;
    float subpixRcpRange = 1.0 / range;
    float subpixNSWE = lumaNS + lumaWE;
    float edgeHorz1 = (-2.0 * lumaM) + lumaNS;
    float edgeVert1 = (-2.0 * lumaM) + lumaWE;

    float lumaNESE = lumaNE + lumaSE;
    float lumaNWNE = lumaNW + lumaNE;
    float edgeHorz2 = (-2.0 * lumaE) + lumaNESE;
    float edgeVert2 = (-2.0 * lumaN) + lumaNWNE;

    float lumaNWSW = lumaNW + lumaSW;
    float lumaSWSE = lumaSW + lumaSE;
    float edgeHorz3 = (-2.0 * lumaW) + lumaNWSW;
    float edgeVert3 = (-2.0 * lumaS) + lumaSWSE;

    float edgeHorz4 = (abs(edgeHorz1) * 2.0) + abs(edgeHorz2);
    float edgeVert4 = (abs(edgeVert1) * 2.0) + abs(edgeVert2);
    float edgeHorz = abs(edgeHorz3) + edgeHorz4;
    float edgeVert = abs(edgeVert3) + edgeVert4;

    float subpixNWSWNESE = lumaNWSW + lumaNESE;
    float lengthSign = rcpFrame.x;
    bool horzSpan = edgeHorz >= edgeVert;
    float subpixA = subpixNSWE * 2.0 + subpixNWSWNESE;

    if (!horzSpan) lumaN = lumaW;
    if (!horzSpan) lumaS = lumaE;
    if (horzSpan) lengthSign = rcpFrame.y;
    
    float subpixB = (subpixA * (1.0/12.0)) - lumaM;

    float gradientN = lumaN - lumaM;
    float gradientS = lumaS - lumaM;
    float lumaNN = lumaN + lumaM;
    float lumaSS = lumaS + lumaM;
    bool pairN = abs(gradientN) >= abs(gradientS);
    float gradient = max(abs(gradientN), abs(gradientS));
    
    if (pairN) lengthSign = -lengthSign;
    float subpixC = saturate(abs(subpixB) * subpixRcpRange);

    float2 posB;
    posB.x = posM.x;
    posB.y = posM.y;
    
    float2 offNP;
    offNP.x = (!horzSpan) ? 0.0 : rcpFrame.x;
    offNP.y = ( horzSpan) ? 0.0 : rcpFrame.y;
    
    if (!horzSpan) posB.x += lengthSign * 0.5;
    if ( horzSpan) posB.y += lengthSign * 0.5;

    float2 posN;
    posN.x = posB.x - offNP.x * 1.0; // 1.0 is P0
    posN.y = posB.y - offNP.y * 1.0;
    float2 posP;
    posP.x = posB.x + offNP.x * 1.0;
    posP.y = posB.y + offNP.y * 1.0;

    float subpixD = ((-2.0) * subpixC) + 3.0;
    float lumaEndN = FxaaLuma(tex.SampleLevel(posN, 0.0));
    float subpixE = subpixC * subpixC;
    float lumaEndP = FxaaLuma(tex.SampleLevel(posP, 0.0));

    if (!pairN) lumaNN = lumaSS;
    float gradientScaled = gradient * 1.0/4.0;
    float lumaMM = lumaM - lumaNN * 0.5;
    float subpixF = subpixD * subpixE;
    bool lumaMLTZero = lumaMM < 0.0;

    lumaEndN -= lumaNN * 0.5;
    lumaEndP -= lumaNN * 0.5;
    
    bool doneN = abs(lumaEndN) >= gradientScaled;
    bool doneP = abs(lumaEndP) >= gradientScaled;
    
    if (!doneN) posN.x -= offNP.x * 1.5; // 1.5 is P1
    if (!doneN) posN.y -= offNP.y * 1.5;
    bool doneNP = (!doneN) || (!doneP);
    if (!doneP) posP.x += offNP.x * 1.5;
    if (!doneP) posP.y += offNP.y * 1.5;

    // 3. Iterative End-of-Edge Search (Quality 39 Steps)
    // --------------------------------------------------
    // Steps: 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0
    // We already did P0(1.0) and P1(1.5).
    // Remaining steps array for the loop:
    float steps[10] = { 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0 };

    if (doneNP) 
    {
        [unroll]
        for (int i = 0; i < 10; i++)
        {
            if (!doneN) lumaEndN = FxaaLuma(tex.SampleLevel(posN.xy, 0.0));
            if (!doneP) lumaEndP = FxaaLuma(tex.SampleLevel(posP.xy, 0.0));
            
            if (!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
            if (!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
            
            bool doneN2 = abs(lumaEndN) >= gradientScaled;
            bool doneP2 = abs(lumaEndP) >= gradientScaled;
            
            if (!doneN) posN.x -= offNP.x * steps[i];
            if (!doneN) posN.y -= offNP.y * steps[i];
            
            doneN = doneN || doneN2;
            doneP = doneP || doneP2;
            
            if (!doneP) posP.x += offNP.x * steps[i];
            if (!doneP) posP.y += offNP.y * steps[i];
            
            if (doneN && doneP) break;
        }
    }

    float dstN = posM.x - posN.x;
    float dstP = posP.x - posM.x;
    if (!horzSpan) dstN = posM.y - posN.y;
    if (!horzSpan) dstP = posP.y - posM.y;

    bool goodSpanN = (lumaEndN < 0.0) != lumaMLTZero;
    float spanLength = (dstP + dstN);
    bool goodSpanP = (lumaEndP < 0.0) != lumaMLTZero;
    float spanLengthRcp = 1.0 / spanLength;

    bool directionN = dstN < dstP;
    float dst = min(dstN, dstP);
    bool goodSpan = directionN ? goodSpanN : goodSpanP;
    float subpixG = subpixF * subpixF;
    float pixelOffset = (dst * (-spanLengthRcp)) + 0.5;
    float subpixH = subpixG * config.subpix;

    float pixelOffsetGood = goodSpan ? pixelOffset : 0.0;
    float pixelOffsetSubpix = max(pixelOffsetGood, subpixH);
    
    if (!horzSpan) posM.x += pixelOffsetSubpix * lengthSign;
    if ( horzSpan) posM.y += pixelOffsetSubpix * lengthSign;

    return float4(tex.SampleLevel(posM, 0.0).xyz, 1.0);
}

[vk::binding(0, 0)] Sampler2D sourceTexture;
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

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target
{
    uint width, height;
    sourceTexture.GetDimensions(width, height);
    float2 rcpFrame = 1.0 / float2(width, height);

    return FxaaPixelShader( input.uv, rcpFrame, sourceTexture, FxaaConfig( 1.00, 0.063, 0.0312));
}