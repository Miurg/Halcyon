#pragma once

#include "HalcyonExport.hpp"
#include <vulkan/vulkan_raii.hpp>
#include "GraphicsCore/VulkanDevice.hpp"
#include "GraphicsCore/Resources/Components/BindlessTextureDSetComponent.hpp"
#include "GraphicsCore/Resources/Managers/Buffer.hpp"
#include "GraphicsCore/Resources/Managers/ResourceHandles.hpp"
#include "GraphicsCore/VulkanConst.hpp"
#include "GraphicsCore/Resources/Managers/DescriptorLayoutRegistry.hpp"

class BufferManager;

// Allocates and updates descriptor sets from a single pool.
// Also owns the DescriptorLayoutRegistry - all layout work goes through here.
class HALCYON_API DescriptorManager
{
public:
	DescriptorManager(VulkanDevice& vulkanDevice);
	~DescriptorManager();

	// Register a named layout.
	void registerLayout(const std::string& name, std::span<const vk::DescriptorSetLayoutBinding> bindings,
	                    vk::DescriptorSetLayoutCreateFlags flags = {}, const void* pNext = nullptr);

	// Resolve a layout name to a raw Vulkan handle.
	vk::DescriptorSetLayout getLayout(const std::string& name) const;

	DSetHandle allocate(const std::string& layoutName, uint32_t count = 1);

	void update(DSetHandle dSet, uint32_t binding, uint32_t copyIndex, vk::DescriptorType type, vk::ImageView view,
	            vk::Sampler sampler, vk::ImageLayout imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
	            uint32_t arrayElement = 0);

	void update(DSetHandle dSet, uint32_t binding, uint32_t copyIndex, vk::DescriptorType type, vk::Buffer buffer,
	            vk::DeviceSize offset = 0, vk::DeviceSize range = VK_WHOLE_SIZE);

	void updateBindlessTextureSet(vk::ImageView textureImageView, vk::Sampler textureSampler,
	                              BindlessTextureDSetComponent& dSetComponent, int textureNumber);
	void updateCubemapDescriptors(BindlessTextureDSetComponent& dSetComponent, vk::ImageView cubemapImageView,
	                              vk::Sampler cubemapSampler, vk::ImageView storageImageView);
	void updateIBLDescriptors(BindlessTextureDSetComponent& dSetComponent, vk::ImageView prefilteredView,
	                          vk::Sampler prefilteredSampler, vk::ImageView brdfLutView, vk::Sampler brdfLutSampler);
	void updateSingleTextureDSet(DSetHandle dIndex, int binding, vk::ImageView imageView, vk::Sampler sampler);
	// Updates only Bindings::Textures::CubemapSampler (binding 3 of textureSet).
	void updateCubemapSamplerDescriptor(BindlessTextureDSetComponent& dSetComponent,
	                                    vk::ImageView cubemapImageView,
	                                    vk::Sampler   cubemapSampler);
	// Updates only Bindings::Textures::GICaptureCubemap (binding 5 of textureSet) — sh_projection input.
	void updateGICaptureCubemapDescriptor(BindlessTextureDSetComponent& dSetComponent,
	                                      vk::ImageView cubemapImageView,
	                                      vk::Sampler   cubemapSampler);
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