// === SET 0 ===

struct CameraData
{
	float4x4 cameraSpaceMatrix;
	float4x4 viewMatrix;
	float4x4 projMatrix;
	float4x4 invViewProj;
	float4 cameraPositionAndPadding;
	float4 frustumPlanes[6];
};
[[vk::binding(0, 0)]]
StructuredBuffer<CameraData> camera;

struct SunUBO
{
	float4x4 lightSpaceMatrix;
	float4 sunDirection;
	float4 sunColor;
	float4 sunAmbient;
	float4 shadowMapSize;
	float4 frustumPlanes[6];
	float shadowCasterRange;
	float3 _pad;
	float4 cameraFrustumLightSpaceBounds;
};
[[vk::binding(1, 0)]]
StructuredBuffer<SunUBO> sun;

// === SET 1 ===

struct ModelData
{
	float4 AABBMin;
	float4 AABBMax;
	uint transformIndex;
	uint materialIndex;
	uint drawCommandIndex;
};
[[vk::binding(0, 1)]]
StructuredBuffer<ModelData> objectBuffer;

struct TransformData
{
	float4x4 model;
};
[[vk::binding(1, 1)]]
StructuredBuffer<TransformData> transformBuffer;

struct VkDrawIndexedIndirectCommand
{
	uint indexCount;
	uint instanceCount;
	uint firstIndex;
	int vertexOffset;
	uint firstInstance;
};
[[vk::binding(2, 1)]]
RWStructuredBuffer<VkDrawIndexedIndirectCommand> indirectDrawBuffer;

[[vk::binding(3, 1)]]
RWStructuredBuffer<uint> visibleIndicesBuffer;

struct PushConstants
{
	uint objectCount;
};
[[vk::push_constant]]
PushConstants push;

bool IsAABBVisible(float4 planes[6], float3 minPos, float3 maxPos)
{
	[unroll]
	for (int i = 0; i < 6; i++)
	{
		float3 p;
		p.x = (planes[i].x > 0) ? maxPos.x : minPos.x;
		p.y = (planes[i].y > 0) ? maxPos.y : minPos.y;
		p.z = (planes[i].z > 0) ? maxPos.z : minPos.z;

		if (dot(planes[i].xyz, p) + planes[i].w < 0) return false;
	}

	return true;
}

struct Ray
{
    float3 origin;
    float3 direction;
};

bool RayIntersectsTriangle(Ray ray, float3 v0, float3 v1, float3 v2)
{
    float3 edge1 = v1 - v0;
    float3 edge2 = v2 - v0;

    float3 h = cross(ray.direction, edge2);
    float a = dot(edge1, h);

    if (a > -0.00001 && a < 0.00001) return false;

    float f = 1.0 / a;
    float3 s = ray.origin - v0;
    float u = f * dot(s, h);

    if (u < 0.0 || u > 1.0) return false;

    float3 q = cross(s, edge1);
    float v = f * dot(ray.direction, q);

    if (v < 0.0 || u + v > 1.0) return false;

    float t = f * dot(edge2, q);
    return t > 0.00001;
}

void GetFrustumCorners(float4x4 invViewProj, out float3 corners[8])
{
    // [0] Near BL
    float4 pt = mul(invViewProj, float4(-1.0, 1.0, 0.0, 1.0));
    corners[0] = pt.xyz / pt.w;
    
    // [1] Near TL
    pt = mul(invViewProj, float4(-1.0, -1.0, 0.0, 1.0));
    corners[1] = pt.xyz / pt.w;
    
    // [2] Near TR
    pt = mul(invViewProj, float4(1.0, -1.0, 0.0, 1.0));
    corners[2] = pt.xyz / pt.w;
    
    // [3] Near BR
    pt = mul(invViewProj, float4(1.0, 1.0, 0.0, 1.0));
    corners[3] = pt.xyz / pt.w;
    
    // [4] Far BL
    pt = mul(invViewProj, float4(-1.0, 1.0, 1.0, 1.0));
    corners[4] = pt.xyz / pt.w;
    
    // [5] Far TL
    pt = mul(invViewProj, float4(-1.0, -1.0, 1.0, 1.0));
    corners[5] = pt.xyz / pt.w;
    
    // [6] Far TR
    pt = mul(invViewProj, float4(1.0, -1.0, 1.0, 1.0));
    corners[6] = pt.xyz / pt.w;
    
    // [7] Far BR
    pt = mul(invViewProj, float4(1.0, 1.0, 1.0, 1.0));
    corners[7] = pt.xyz / pt.w;
}

bool DoesRayIntersectFrustum(Ray ray, float3 frustumCorners[8])
{
    // Left side
    if (RayIntersectsTriangle(ray, frustumCorners[0], frustumCorners[1], frustumCorners[5])) return true;
    if (RayIntersectsTriangle(ray, frustumCorners[0], frustumCorners[5], frustumCorners[4])) return true;
    // Right side
    if (RayIntersectsTriangle(ray, frustumCorners[2], frustumCorners[3], frustumCorners[7])) return true;
    if (RayIntersectsTriangle(ray, frustumCorners[2], frustumCorners[7], frustumCorners[6])) return true;
    // Top side
    if (RayIntersectsTriangle(ray, frustumCorners[1], frustumCorners[2], frustumCorners[6])) return true;
    if (RayIntersectsTriangle(ray, frustumCorners[1], frustumCorners[6], frustumCorners[5])) return true;
    // Bottom side
    if (RayIntersectsTriangle(ray, frustumCorners[0], frustumCorners[3], frustumCorners[7])) return true;
    if (RayIntersectsTriangle(ray, frustumCorners[0], frustumCorners[7], frustumCorners[4])) return true;
    // Near plane
    if (RayIntersectsTriangle(ray, frustumCorners[0], frustumCorners[1], frustumCorners[2])) return true;
    if (RayIntersectsTriangle(ray, frustumCorners[2], frustumCorners[3], frustumCorners[0])) return true;
    // Far plane
    if (RayIntersectsTriangle(ray, frustumCorners[4], frustumCorners[5], frustumCorners[6])) return true;
    if (RayIntersectsTriangle(ray, frustumCorners[6], frustumCorners[7], frustumCorners[4])) return true;

    return false;
}

bool RayIntersectsAABB(Ray ray, float3 minBound, float3 maxBound)
{
    float3 invDir = 1.0 / ray.direction;
    float3 t0 = (minBound - ray.origin) * invDir;
    float3 t1 = (maxBound - ray.origin) * invDir;

    float3 tmin = min(t0, t1);
    float3 tmax = max(t0, t1);

    float tNear = max(max(tmin.x, tmin.y), tmin.z);
    float tFar = min(min(tmax.x, tmax.y), tmax.z);

    return tNear <= tFar && tFar >= 0.0;
}

[shader("compute")]
[numthreads(64, 1, 1)]
void computeMain(uint3 id: SV_DispatchThreadID)
{
	uint index = id.x;
	if (index >= push.objectCount) return;

	ModelData obj = objectBuffer[index];
	TransformData trans = transformBuffer[obj.transformIndex];

	float3 localCenter  = (obj.AABBMax.xyz + obj.AABBMin.xyz) * 0.5;
	float3 localExtents = (obj.AABBMax.xyz - obj.AABBMin.xyz) * 0.5;

	float3 worldCenter  = mul(trans.model, float4(localCenter, 1.0)).xyz;
	float3 right        = abs(mul((float3x3)trans.model, float3(1, 0, 0))) * localExtents.x;
	float3 up           = abs(mul((float3x3)trans.model, float3(0, 1, 0))) * localExtents.y;
	float3 forward      = abs(mul((float3x3)trans.model, float3(0, 0, 1))) * localExtents.z;
	float3 worldExtents = right + up + forward;

	float3 worldMin = worldCenter - worldExtents;
	float3 worldMax = worldCenter + worldExtents;

    if (!IsAABBVisible(sun[0].frustumPlanes, worldMin, worldMax)) return;

    bool castsShadowIntoView = false;
    
    if (IsAABBVisible(camera[0].frustumPlanes, worldMin, worldMax)) 
    {
        castsShadowIntoView = true;
    }
    else 
    {
        float3 frustumCorners[8];
        GetFrustumCorners(camera[0].invViewProj, frustumCorners);
        
        float3 lightDir = -sun[0].sunDirection.xyz;
        Ray ray;
        ray.direction = lightDir;
        
        // 1. Fast center test
        ray.origin = worldCenter;
        if (DoesRayIntersectFrustum(ray, frustumCorners)) 
        {
            castsShadowIntoView = true;
        }
        else 
        {
            // 2.Test all 8 AABB corners
            float3 aabbCorners[8] = {
                float3(worldMin.x, worldMin.y, worldMin.z),
                float3(worldMin.x, worldMin.y, worldMax.z),
                float3(worldMin.x, worldMax.y, worldMin.z),
                float3(worldMin.x, worldMax.y, worldMax.z),
                float3(worldMax.x, worldMin.y, worldMin.z),
                float3(worldMax.x, worldMin.y, worldMax.z),
                float3(worldMax.x, worldMax.y, worldMin.z),
                float3(worldMax.x, worldMax.y, worldMax.z)
            };

            for (int i = 0; i < 8; i++) 
            {
                ray.origin = aabbCorners[i];
                if (DoesRayIntersectFrustum(ray, frustumCorners)) 
                {
                    castsShadowIntoView = true;
                    break;
                }
            }
        }

        // 3. If still not intersecting, 
        //test if the object is between the camera and the sun 
        // by casting a ray from the frustum corners towards the sun 
        // and checking for intersection with the object's AABB
        if (!castsShadowIntoView) 
        {
            Ray revRay;
            revRay.direction = -lightDir; 
            for (int i = 0; i < 8; ++i) 
            {
                revRay.origin = frustumCorners[i];
                if (RayIntersectsAABB(revRay, worldMin, worldMax)) 
                {
                    castsShadowIntoView = true;
                    break;
                }
            }
        }
    }

    if (!castsShadowIntoView) return;

	uint drawCommandIndex    = obj.drawCommandIndex;
	uint visibleBufferOffset = indirectDrawBuffer[drawCommandIndex].firstInstance;

	uint slotIndex;
	InterlockedAdd(indirectDrawBuffer[drawCommandIndex].instanceCount, 1, slotIndex);
	visibleIndicesBuffer[slotIndex + visibleBufferOffset] = index;
}