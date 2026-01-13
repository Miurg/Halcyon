#pragma once
#include <vulkan/vulkan_raii.hpp>

struct MaterialDSetComponent
{
	vk::Sampler textureSampler = nullptr;
	vk::DescriptorSet bindlessTextureSet = nullptr;
};