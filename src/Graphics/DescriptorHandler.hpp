#pragma once

#include <vulkan/vulkan_raii.hpp>
#include "VulkanDevice.hpp"
#include "Resources/Managers/Texture.hpp"
#include "SwapChain.hpp"
#include "GameObject.hpp"
#include "FrameData.hpp"

class DescriptorHandler
{
public:
	vk::raii::DescriptorPool descriptorPool = nullptr;
	std::vector<vk::raii::DescriptorSet> descriptorSets;
	vk::raii::DescriptorSetLayout cameraSetLayout = nullptr;
	vk::raii::DescriptorSetLayout textureSetLayout = nullptr;
	vk::raii::DescriptorSetLayout modelSetLayout = nullptr;
};

struct CameraUBO
{
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct ModelUBO
{
	alignas(16) glm::mat4 model;
};