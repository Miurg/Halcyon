module SH;

public float3 computeIrradiance(float3 n, float3 sh[4])
{
    const float f0 = 0.28209479f;
    const float f1 = 0.48860251f;
    const float f3 = 0.31539156f;

    float3 color = sh[0] * f0;
    color += sh[1] * (-f1 * n.y);
    color += sh[2] * (f1 * n.z);
    color += sh[3] * (-f1 * n.x);

    float3 vx = -sh[3];
    float3 vy = -sh[1];
    float3 vz = sh[2];

    const float3 lum = float3(0.2126f, 0.7152f, 0.0722f);
    float3 axis = float3(dot(vx, lum), dot(vy, lum), dot(vz, lum));
    float axisLen = length(axis);
    if (axisLen > 1e-6f)
    {
        axis /= axisLen;
        float3 proj = abs(float3(dot(float3(vx.r, vy.r, vz.r), axis),
                                 dot(float3(vx.g, vy.g, vz.g), axis),
                                 dot(float3(vx.b, vy.b, vz.b), axis)));
        float3 ratio = 1.5f * proj / max(sh[0], float3(1e-4f));
        float3 zonal = 0.25f * sh[0] * (0.08f * ratio + 0.6f * ratio * ratio);
        float fZ = dot(axis, n);
        color += zonal * (f3 * (3.0f * fZ * fZ - 1.0f));
    }

    return max(color, float3(0.0));
}
