#pragma once
#include "../../RenderCore/Camera.h"
#include "ComponentManager.h"

class MeshAsset;
class MaterialAsset;

struct RenderableComponent : Component
{
	MeshAsset* Mesh;
	MaterialAsset* Material;
	uint32_t batchIndex = UINT32_MAX;
	RenderableComponent(MeshAsset* m, MaterialAsset* mat) : Mesh(m), Material(mat) {}
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