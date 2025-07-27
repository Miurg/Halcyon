#pragma once
#include "ComponentManager.h"

// Forward declarations
class MeshAsset;
class MaterialAsset;

struct TransformComponent : Component 
{
    glm::vec3 Position = glm::vec3(0.0f);
    glm::quat Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); 
    glm::vec3 Scale = glm::vec3(1.0f);
    
    TransformComponent() = default;
    TransformComponent(glm::vec3 pos) : Position(pos) {}
    TransformComponent(glm::vec3 pos, glm::quat rot) : Position(pos), Rotation(rot) {}
    TransformComponent(glm::vec3 pos, glm::quat rot, glm::vec3 scl) : Position(pos), Rotation(rot), Scale(scl) {}
};

struct RenderableComponent : Component 
{
    MeshAsset* Mesh;
    MaterialAsset* Material;
    
    RenderableComponent(MeshAsset* m, MaterialAsset* mat) 
        : Mesh(m), Material(mat) {}
};

struct VelocityComponent : Component 
{
    glm::vec3 Velocity;
    VelocityComponent(glm::vec3 vel) : Velocity(vel) {}
};

struct RotationSpeedComponent : Component 
{
    float Speed = 0.0f;
    glm::vec3 Axis = glm::vec3(0.0f, 1.0f, 0.0f);
    RotationSpeedComponent(float s, glm::vec3 ax) : Speed(s), Axis(ax) {}
};

struct CameraComponent : Component 
{
    Camera* Cam;
    CameraComponent(Camera* cam) : Cam(cam) {}
};