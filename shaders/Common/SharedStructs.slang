module SharedStructs;

public struct GpuMaterialData
{
    public float alphaCutoff;
    public uint alphaMode;
    public uint doubleSided;
}

public struct SurfaceContext
{
    public GpuMaterialData mat;
    public float3 albedo = float3(1.0f, 1.0f, 1.0f);
    public float alpha = 1.0;
    public float3 emissive = float3(1.0f, 1.0f, 1.0f);
    public float roughness = 1.0;
    public float3 normal;
    public float metallic = 1.0;
    public float3 F0;
    public float NdotV;
    public float3 viewVector;
};