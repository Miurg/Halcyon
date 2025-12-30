#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "VulkanConst.hpp"
#include "VulkanDevice.hpp"
#include "VulkanUtils.hpp"
#include "Resources/Managers/Texture.hpp"
#include <iostream>
#include "Resources/Components/MeshInfoComponent.hpp"


struct GameObject
{
	vk::raii::DescriptorSet textureDescriptorSet = nullptr;
};