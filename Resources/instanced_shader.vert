#version 460 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

struct InstanceTransform 
{
    vec4 position;
    vec4 rotation; // (x, y, z, w)
    vec4 scale;
};

layout(std430, binding = 0) buffer InstanceBuffer 
{
    InstanceTransform transforms[];
};

uniform mat4 view;
uniform mat4 projection;
uniform int baseInstance;

out vec2 TexCoord;

mat4 quatToMat4(vec4 q) 
{
    q = normalize(q);
    float x = q.x, y = q.y, z = q.z, w = q.w;
    float x2 = x + x, y2 = y + y, z2 = z + z;
    float xx = x * x2, xy = x * y2, xz = x * z2;
    float yy = y * y2, yz = y * z2, zz = z * z2;
    float wx = w * x2, wy = w * y2, wz = w * z2;

    return mat4(
        1.0 - (yy + zz), xy + wz, xz - wy, 0.0,
        xy - wz, 1.0 - (xx + zz), yz + wx, 0.0,
        xz + wy, yz - wx, 1.0 - (xx + yy), 0.0,
        0.0, 0.0, 0.0, 1.0
    );
}

void main()
{
   uint instanceIndex = gl_InstanceID + uint(baseInstance);
    InstanceTransform t = transforms[instanceIndex];

    vec3 scaledPos = inPosition * t.scale.xyz;
    
    vec4 q = normalize(t.rotation);
    vec3 rotatedPos = scaledPos + 2.0 * cross(q.xyz, cross(q.xyz, scaledPos) + q.w * scaledPos);
    
    vec3 worldPos = rotatedPos + t.position.xyz;

    gl_Position = projection * view * vec4(worldPos, 1.0);
    TexCoord = inTexCoord;
}