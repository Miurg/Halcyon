#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan_raii.hpp>
#include "../GameObject.hpp"

struct GameObjectComponent
{
	GameObject* objectInstance;

	GameObjectComponent(GameObject* object) : objectInstance(object) {}
};