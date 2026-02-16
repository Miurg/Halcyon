#pragma once
#include <vulkan/vulkan_raii.hpp>
#include "../../VulkanDevice.hpp"
#include "../Components/BindlessTextureDSetComponent.hpp"
#include "Buffer.hpp"
#include "ResourceHandles.hpp"
#include "../../VulkanConst.hpp"

class BufferManager;

class DescriptorManager
{
public:
	DescriptorManager(VulkanDevice& vulkanDevice);
	~DescriptorManager();
	vk::raii::DescriptorSetLayout globalSetLayout = nullptr;
	vk::raii::DescriptorSetLayout textureSetLayout = nullptr;
	vk::raii::DescriptorSetLayout modelSetLayout = nullptr;
	vk::raii::DescriptorSetLayout fxaaSetLayout = nullptr;

	DSetHandle allocateBindlessTextureDSet();
	void updateBindlessTextureSet(vk::ImageView textureImageView, vk::Sampler textureSampler,
	                              BindlessTextureDSetComponent& dSetComponent, int textureNumber);
	void updateSingleTextureDSet(DSetHandle dIndex, int binding, vk::ImageView imageView, vk::Sampler sampler);
	DSetHandle allocateStorageBufferDSets(uint32_t count, vk::DescriptorSetLayout layout);
	void updateStorageBufferDescriptors(BufferManager& bManager, BufferHandle bNumber, DSetHandle dSet,
	                                    uint32_t binding);
	DSetHandle allocateFxaaDescriptorSet(vk::DescriptorSetLayout layout);
	std::vector<std::vector<vk::DescriptorSet>> descriptorSets;

private:
	VulkanDevice& vulkanDevice;
	vk::raii::DescriptorPool descriptorPool = nullptr;
};