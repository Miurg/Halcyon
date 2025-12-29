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
	vk::raii::DescriptorSetLayout uboSetLayout = nullptr; 
	vk::raii::DescriptorSetLayout textureSetLayout = nullptr;
};