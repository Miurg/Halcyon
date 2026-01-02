#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc.h>

struct Texture
{
	vk::Image textureImage;
	VmaAllocation textureImageAllocation;
	vk::ImageView textureImageView;
	vk::Sampler textureSampler;
	vk::DescriptorSet textureDescriptorSet;
};