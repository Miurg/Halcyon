// === Set 0: exposureSet ===
[vk::binding(0, 0)]
StructuredBuffer<uint> histogramBuffer;

[vk::binding(1, 0)]
RWStructuredBuffer<float> exposureBuffer;

struct PushConstants {
    float deltaTime;
    float tauUp;
    float tauDown;
    float minEV;
    float maxEV;
    float targetLuminance;
    float lowPercent;
    float highPercent;
    uint pixelCount;
    float minLogLum;
    float logLumRange;
};

[[vk::push_constant]]
PushConstants push;

[shader("compute")]
[numthreads(1, 1, 1)]
void computeMain(uint3 id: SV_DispatchThreadID)
{
    // Compute CDF from the histogram
    uint cdf[256];
    cdf[0] = histogramBuffer[0];
    for (int i = 1; i < 256; i++) {
        cdf[i] = cdf[i - 1] + histogramBuffer[i];
    }

    // Find the luminance values corresponding to the low and high percentiles
    uint lowCount = uint(push.lowPercent * push.pixelCount);
    uint highCount = uint(push.highPercent * push.pixelCount);

    float lowLogLum = push.minLogLum;
    float highLogLum = push.minLogLum + push.logLumRange;
    for (int i = 0; i < 256; i++) {
        if (cdf[i] >= lowCount) {
            lowLogLum = push.minLogLum + (i / 255.0) * push.logLumRange;
            break;
        }
    }
    for (int i = 0; i < 256; i++) {
        if (cdf[i] >= highCount) {
            highLogLum = push.minLogLum + (i / 255.0) * push.logLumRange;
            break;
        }
    }

    float targetExposure = pow(2.0, push.targetLuminance - ((lowLogLum + highLogLum) / 2.0));
     
    // Smoothly adjust the current exposure towards the target exposure
    float currentExposure = exposureBuffer[0];
    if (targetExposure > currentExposure) {
        currentExposure += (targetExposure - currentExposure) * (1.0 - exp(-push.deltaTime / push.tauUp));
    } else {
        currentExposure += (targetExposure - currentExposure) * (1.0 - exp(-push.deltaTime / push.tauDown));
    }

    // Clamp
    currentExposure = clamp(currentExposure, pow(2.0, push.minEV), pow(2.0, push.maxEV));

    exposureBuffer[0] = currentExposure;
}