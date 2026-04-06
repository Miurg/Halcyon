#pragma once
#include <vulkan/vulkan_raii.hpp>
#include "../../VulkanDevice.hpp"
#include "../Components/BindlessTextureDSetComponent.hpp"
#include "Buffer.hpp"
#include "ResourceHandles.hpp"
#include "../../VulkanConst.hpp"

class BufferManager;

// Allocates and updates descriptor sets from a single pool (bindless textures, storage buffers, FXAA).
class DescriptorManager
{
public:
	DescriptorManager(VulkanDevice& vulkanDevice);
	~DescriptorManager();
	vk::raii::DescriptorSetLayout globalSetLayout = nullptr;
	vk::raii::DescriptorSetLayout textureSetLayout = nullptr;
	vk::raii::DescriptorSetLayout modelSetLayout = nullptr;
	vk::raii::DescriptorSetLayout screenSpaceSetLayout = nullptr;

	DSetHandle allocateBindlessTextureDSet();
	void updateBindlessTextureSet(vk::ImageView textureImageView, vk::Sampler textureSampler,
	                              BindlessTextureDSetComponent& dSetComponent, int textureNumber);
	void updateCubemapDescriptors(BindlessTextureDSetComponent& dSetComponent, vk::ImageView cubemapImageView,
	                              vk::Sampler cubemapSampler, vk::ImageView storageImageView);
	void updateIBLDescriptors(BindlessTextureDSetComponent& dSetComponent, vk::ImageView prefilteredView,
	                          vk::Sampler prefilteredSampler, vk::ImageView brdfLutView, vk::Sampler brdfLutSampler);
	void updateSHBufferDescriptor(BindlessTextureDSetComponent& dSetComponent, vk::Buffer shBuffer,
	                              vk::DeviceSize bufferSize);
	void updateSingleTextureDSet(DSetHandle dIndex, int binding, vk::ImageView imageView, vk::Sampler sampler);
	DSetHandle allocateStorageBufferDSets(uint32_t count, vk::DescriptorSetLayout layout);
	void updateStorageBufferDescriptors(BufferManager& bManager, BufferHandle bNumber, DSetHandle dSet,
	                                    uint32_t binding);
	DSetHandle allocateOffscreenDescriptorSet(vk::DescriptorSetLayout layout);

	// Returns the descriptor set at the given slot index (0 for single-buffered, currentFrame for per-frame sets).
	vk::DescriptorSet getSet(DSetHandle handle, uint32_t index = 0) const
	{
		return descriptorSets[handle.id][index];
	}

	// Returns the number of buffered copies in this set (1 for single-frame, MAX_FRAMES_IN_FLIGHT for per-frame).
	uint32_t getSetCount(DSetHandle handle) const
	{
		return static_cast<uint32_t>(descriptorSets[handle.id].size());
	}

	vk::raii::DescriptorPool imguiPool = nullptr;

private:
	VulkanDevice& vulkanDevice;
	vk::raii::DescriptorPool descriptorPool = nullptr;
	std::vector<std::vector<vk::DescriptorSet>> descriptorSets;
};