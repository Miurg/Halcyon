#pragma once
#include <vulkan/vulkan_raii.hpp>
#include "../../VulkanDevice.hpp"
#include "../Components/MaterialDSetComponent.hpp"
#include "Buffer.hpp"
#include "../../VulkanConst.hpp"

class BufferManager;

class DescriptorManager
{
public:
	DescriptorManager(VulkanDevice& vulkanDevice);
	vk::raii::DescriptorSetLayout globalSetLayout = nullptr;
	vk::raii::DescriptorSetLayout textureSetLayout = nullptr;
	vk::raii::DescriptorSetLayout modelSetLayout = nullptr;
	vk::Sampler textureSampler;
	vk::DescriptorSet bindlessTextureSet;

	void allocateMaterialDSet(MaterialDSetComponent& dSetComponent);
	void allocateGlobalDescriptorSets(Buffer& bufferIn, size_t sizeBuffer, uint_fast16_t numberBuffers,
	                                  uint_fast16_t numberBinding, vk::DescriptorSetLayout layout);
	void updateBindlessTextureSet(int textureNumber, MaterialDSetComponent& dSetComponent, BufferManager& bManager);
	void bindShadowMap(int bufferIndex, vk::ImageView imageView, vk::Sampler sampler, BufferManager& bManager);

private:
	VulkanDevice& vulkanDevice;
	vk::raii::DescriptorPool descriptorPool = nullptr;
};