module Sphere;

// UV sphere: N_LAT rings x N_LON segments, 2 triangles per quad = 6 verts per quad
public static const int N_LAT = 8;
public static const int N_LON = 8;

public float3 spherePoint(float phi, float theta)
{
    return float3(sin(phi) * cos(theta), cos(phi), sin(phi) * sin(theta));
}
