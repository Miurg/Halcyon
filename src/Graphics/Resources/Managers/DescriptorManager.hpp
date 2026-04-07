#pragma once
#include <vulkan/vulkan_raii.hpp>
#include "../../VulkanDevice.hpp"
#include "../Components/BindlessTextureDSetComponent.hpp"
#include "Buffer.hpp"
#include "ResourceHandles.hpp"
#include "../../VulkanConst.hpp"
#include "DescriptorLayoutRegistry.hpp"

class BufferManager;

// Allocates and updates descriptor sets from a single pool.
// Also owns the DescriptorLayoutRegistry - all layout work goes through here.
class DescriptorManager
{
public:
	DescriptorManager(VulkanDevice& vulkanDevice);
	~DescriptorManager();

	// Register a named layout.
	void registerLayout(const std::string& name, std::span<const vk::DescriptorSetLayoutBinding> bindings,
	                    vk::DescriptorSetLayoutCreateFlags flags = {}, const void* pNext = nullptr);

	// Resolve a layout name to a raw Vulkan handle.
	vk::DescriptorSetLayout getLayout(const std::string& name) const;

	DSetHandle allocateBindlessTextureDSet();
	DSetHandle allocateStorageBufferDSets(uint32_t count, const std::string& layoutName);
	DSetHandle allocateOffscreenDescriptorSet(const std::string& layoutName);

	void updateBindlessTextureSet(vk::ImageView textureImageView, vk::Sampler textureSampler,
	                              BindlessTextureDSetComponent& dSetComponent, int textureNumber);
	void updateCubemapDescriptors(BindlessTextureDSetComponent& dSetComponent, vk::ImageView cubemapImageView,
	                              vk::Sampler cubemapSampler, vk::ImageView storageImageView);
	void updateIBLDescriptors(BindlessTextureDSetComponent& dSetComponent, vk::ImageView prefilteredView,
	                          vk::Sampler prefilteredSampler, vk::ImageView brdfLutView, vk::Sampler brdfLutSampler);
	void updateSHBufferDescriptor(BindlessTextureDSetComponent& dSetComponent, vk::Buffer shBuffer,
	                              vk::DeviceSize bufferSize);
	void updateSingleTextureDSet(DSetHandle dIndex, int binding, vk::ImageView imageView, vk::Sampler sampler);
	void updateStorageBufferDescriptors(BufferManager& bManager, BufferHandle bNumber, DSetHandle dSet,
	                                    uint32_t binding);

	// Returns the set at the given slot index (0 for single-buffered, currentFrame for per-frame).
	vk::DescriptorSet getSet(DSetHandle handle, uint32_t index = 0) const
	{
		return descriptorSets[handle.id][index];
	}

	// Returns the number of buffered copies.
	uint32_t getSetCount(DSetHandle handle) const
	{
		return static_cast<uint32_t>(descriptorSets[handle.id].size());
	}

	vk::raii::DescriptorPool imguiPool = nullptr;

private:
	VulkanDevice& vulkanDevice;
	DescriptorLayoutRegistry layoutRegistry;
	vk::raii::DescriptorPool descriptorPool = nullptr;
	std::vector<std::vector<vk::DescriptorSet>> descriptorSets;
};