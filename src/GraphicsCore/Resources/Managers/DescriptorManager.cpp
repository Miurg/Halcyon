#include "GraphicsCore/Resources/Managers/DescriptorManager.hpp"
#include "Shared/Bindings.h"

DescriptorManager::DescriptorManager(VulkanDevice& vulkanDevice)
    : vulkanDevice(vulkanDevice), layoutRegistry(vulkanDevice.device)
{
	std::array poolSize{vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1000 * MAX_FRAMES_IN_FLIGHT),
	                    vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler,
	                                           1000 * MAX_FRAMES_IN_FLIGHT + MAX_BINDLESS_TEXTURES),
	                    vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 1000 * MAX_FRAMES_IN_FLIGHT)};

	vk::DescriptorPoolCreateInfo poolInfo;
	poolInfo.flags =
	    vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet | vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;
	poolInfo.maxSets = 1000 * 3 * MAX_FRAMES_IN_FLIGHT + 1;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSize.size());
	poolInfo.pPoolSizes = poolSize.data();

	descriptorPool = vk::raii::DescriptorPool(vulkanDevice.device, poolInfo);

	// Set 0: Global
	{
		using S = vk::ShaderStageFlagBits;
		constexpr auto kAllStages = S::eVertex | S::eFragment | S::eCompute;
		std::array globalBindings = {
		    vk::DescriptorSetLayoutBinding(BIND_GLOBAL_CAMERA, vk::DescriptorType::eStorageBuffer, 1, kAllStages),
		    vk::DescriptorSetLayoutBinding(BIND_GLOBAL_SUN, vk::DescriptorType::eStorageBuffer, 1, kAllStages),
		    vk::DescriptorSetLayoutBinding(BIND_GLOBAL_POINT_LIGHTS, vk::DescriptorType::eStorageBuffer, 1,
		                                   kAllStages),
		    vk::DescriptorSetLayoutBinding(BIND_GLOBAL_POINT_LIGHT_COUNT, vk::DescriptorType::eStorageBuffer, 1,
		                                   kAllStages),
		    vk::DescriptorSetLayoutBinding(BIND_GLOBAL_SH_PROBES, vk::DescriptorType::eStorageBuffer, 1, kAllStages),
		    vk::DescriptorSetLayoutBinding(BIND_GLOBAL_SH_GRID_INFO, vk::DescriptorType::eStorageBuffer, 1,
		                                   kAllStages),
		    vk::DescriptorSetLayoutBinding(BIND_GLOBAL_GTAO_TEXTURE, vk::DescriptorType::eCombinedImageSampler, 1,
		                                   S::eFragment),
		    vk::DescriptorSetLayoutBinding(BIND_GLOBAL_REFLECTION_PROBES, vk::DescriptorType::eStorageBuffer, 1,
		                                   kAllStages),
		    vk::DescriptorSetLayoutBinding(BIND_GLOBAL_REFLECTION_PROBE_COUNT, vk::DescriptorType::eStorageBuffer, 1,
		                                   kAllStages),
		    vk::DescriptorSetLayoutBinding(BIND_GLOBAL_FORWARD_CLUSTERED_GRID, vk::DescriptorType::eStorageBuffer, 1,
		                                   kAllStages),
		    vk::DescriptorSetLayoutBinding(BIND_GLOBAL_FORWARD_CLUSTERED_INFO, vk::DescriptorType::eStorageBuffer, 1,
		                                   kAllStages),
		    vk::DescriptorSetLayoutBinding(BIND_GLOBAL_VISIBLE_POINT_LIGHTS, vk::DescriptorType::eStorageBuffer, 1,
		                                   S::eCompute | S::eFragment),
		};
		registerLayout("globalSet", globalBindings);
	}

	// Set 1: Model
	{
		using S = vk::ShaderStageFlagBits;
		std::array modelBindings = {
		    vk::DescriptorSetLayoutBinding(BIND_MODEL_PRIMITIVES, vk::DescriptorType::eStorageBuffer, 1,
		                                   S::eVertex | S::eCompute),
		    vk::DescriptorSetLayoutBinding(BIND_MODEL_TRANSFORMS, vk::DescriptorType::eStorageBuffer, 1,
		                                   S::eVertex | S::eCompute),
		    vk::DescriptorSetLayoutBinding(BIND_MODEL_INDIRECT_DRAW, vk::DescriptorType::eStorageBuffer, 1,
		                                   S::eCompute),
		    vk::DescriptorSetLayoutBinding(BIND_MODEL_VISIBLE_INDICES, vk::DescriptorType::eStorageBuffer, 1,
		                                   S::eCompute | S::eVertex),
		    vk::DescriptorSetLayoutBinding(BIND_MODEL_COMPACTED_DRAW, vk::DescriptorType::eStorageBuffer, 1,
		                                   S::eCompute),
		    vk::DescriptorSetLayoutBinding(BIND_MODEL_DRAW_COUNT, vk::DescriptorType::eStorageBuffer, 1,
		                                   S::eCompute | S::eVertex),
		};
		registerLayout("modelSet", modelBindings);
	}

	// Set 2: Textures
	{
		using S = vk::ShaderStageFlagBits;
		std::array textureBindings = {
		    vk::DescriptorSetLayoutBinding(BIND_TEXTURES_ARRAY, vk::DescriptorType::eCombinedImageSampler,
		                                   MAX_BINDLESS_TEXTURES, S::eFragment | S::eCompute),
		    vk::DescriptorSetLayoutBinding(BIND_TEXTURES_SHADOW_MAP, vk::DescriptorType::eCombinedImageSampler, 1,
		                                   S::eFragment),
		    vk::DescriptorSetLayoutBinding(BIND_TEXTURES_MATERIALS, vk::DescriptorType::eStorageBuffer, 1,
		                                   S::eVertex | S::eFragment),
		    vk::DescriptorSetLayoutBinding(BIND_TEXTURES_CUBEMAP_SAMPLER, vk::DescriptorType::eCombinedImageSampler,
		                                   1, S::eFragment | S::eCompute),
		    vk::DescriptorSetLayoutBinding(BIND_TEXTURES_CUBEMAP_STORAGE, vk::DescriptorType::eStorageImage, 1,
		                                   S::eCompute),
		    vk::DescriptorSetLayoutBinding(BIND_TEXTURES_GI_CAPTURE_CUBEMAP,
		                                   vk::DescriptorType::eCombinedImageSampler, 1, S::eCompute),
		    vk::DescriptorSetLayoutBinding(BIND_TEXTURES_PREFILTERED_MAP, vk::DescriptorType::eCombinedImageSampler,
		                                   1, S::eFragment),
		    vk::DescriptorSetLayoutBinding(BIND_TEXTURES_BRDF_LUT, vk::DescriptorType::eCombinedImageSampler, 1,
		                                   S::eFragment),
		    vk::DescriptorSetLayoutBinding(BIND_TEXTURES_REFLECTION_CUBEMAPS,
		                                   vk::DescriptorType::eCombinedImageSampler, MAX_REFLECTION_PROBES,
		                                   S::eFragment),
		};
		std::array<vk::DescriptorBindingFlags, 9> textureBindingFlags = {
		    vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eUpdateAfterBind,
		    vk::DescriptorBindingFlags{}, // shadowMap
		    vk::DescriptorBindingFlags{}, // materials
		    vk::DescriptorBindingFlags{}, // cubemapSampler
		    vk::DescriptorBindingFlags{}, // cubemapStorage
		    vk::DescriptorBindingFlags{}, // giCaptureCubemap
		    vk::DescriptorBindingFlags{}, // prefilteredMap
		    vk::DescriptorBindingFlags{}, // brdfLut
		    vk::DescriptorBindingFlagBits::ePartiallyBound |
		        vk::DescriptorBindingFlagBits::eUpdateAfterBind, // reflectionCubemaps
		};
		vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo;
		bindingFlagsInfo.bindingCount = static_cast<uint32_t>(textureBindingFlags.size());
		bindingFlagsInfo.pBindingFlags = textureBindingFlags.data();
		registerLayout("textureSet", textureBindings, vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool,
		               &bindingFlagsInfo);
	}

	// Shared fullscreen layout: 3 CombinedImageSampler slots
	{
		std::array<vk::DescriptorSetLayoutBinding, 3> ssBindings{};
		for (uint32_t i = 0; i < 3; i++)
		{
			ssBindings[i].binding = i;
			ssBindings[i].descriptorCount = 1;
			ssBindings[i].descriptorType = vk::DescriptorType::eCombinedImageSampler;
			ssBindings[i].stageFlags = vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute;
		}
		registerLayout("screenSpaceSet", ssBindings);
	}

	{
		using S = vk::ShaderStageFlagBits;
		std::array depthPyramidBindings = {
		    vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eCombinedImageSampler, 1, S::eCompute),
		    vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageImage, 1, S::eCompute)};
		std::array<vk::DescriptorBindingFlags, 2> depthPyramidBindingFlags = {vk::DescriptorBindingFlags{},
		                                                                      vk::DescriptorBindingFlags{}};
		vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo;
		bindingFlagsInfo.bindingCount = static_cast<uint32_t>(depthPyramidBindingFlags.size());
		bindingFlagsInfo.pBindingFlags = depthPyramidBindingFlags.data();
		registerLayout("hiZSet", depthPyramidBindings);
	}

	using S = vk::ShaderStageFlagBits;
	std::array exposureBindings = {
	    vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, S::eCompute),
	    vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, S::eCompute | S::eFragment),
	};
	registerLayout("exposureSet", exposureBindings);

	using S = vk::ShaderStageFlagBits;
	std::array particleSystemBindings = {
	    vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1,
	                                   S::eCompute | S::eFragment | S::eVertex),
	    vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, S::eCompute),
	    vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, S::eCompute),
	    vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, 1, S::eCompute),
	    vk::DescriptorSetLayoutBinding(4, vk::DescriptorType::eStorageBuffer, 1, S::eCompute),
	    vk::DescriptorSetLayoutBinding(5, vk::DescriptorType::eStorageBuffer, 1, S::eCompute | S::eVertex),
	    vk::DescriptorSetLayoutBinding(6, vk::DescriptorType::eStorageBuffer, 1, S::eCompute | S::eVertex),
	    vk::DescriptorSetLayoutBinding(7, vk::DescriptorType::eStorageBuffer, 1, S::eCompute),
	    vk::DescriptorSetLayoutBinding(8, vk::DescriptorType::eStorageBuffer, 1, S::eCompute),
	    vk::DescriptorSetLayoutBinding(9, vk::DescriptorType::eStorageBuffer, 1, S::eCompute),
	};
	registerLayout("particleSystemSet", particleSystemBindings);

} // Still mess.

void DescriptorManager::registerLayout(const std::string& name,
                                       std::span<const vk::DescriptorSetLayoutBinding> bindings,
                                       vk::DescriptorSetLayoutCreateFlags flags, const void* pNext)
{
	layoutRegistry.create(name, bindings, flags, pNext);
}

vk::DescriptorSetLayout DescriptorManager::getLayout(const std::string& name) const
{
	return layoutRegistry.get(name);
}

DSetHandle DescriptorManager::allocate(const std::string& layoutName, uint32_t count)
{
	vk::DescriptorSetLayout layout = layoutRegistry.get(layoutName);
	std::vector<vk::DescriptorSetLayout> layouts(count, layout);

	vk::DescriptorSetAllocateInfo allocInfo{};
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = count;
	allocInfo.pSetLayouts = layouts.data();

	auto allocatedSets = (*vulkanDevice.device).allocateDescriptorSets(allocInfo);
	descriptorSets.push_back(std::move(allocatedSets));
	return DSetHandle{static_cast<int>(descriptorSets.size() - 1)};
}

void DescriptorManager::update(DSetHandle dSet, uint32_t binding, uint32_t copyIndex, vk::DescriptorType type,
                               vk::ImageView view, vk::Sampler sampler, vk::ImageLayout imageLayout,
                               uint32_t arrayElement)
{
	vk::DescriptorImageInfo imageInfo;
	imageInfo.sampler = sampler;
	imageInfo.imageView = view;
	imageInfo.imageLayout = imageLayout;

	vk::WriteDescriptorSet write;
	write.dstSet = descriptorSets[dSet.id][copyIndex];
	write.dstBinding = binding;
	write.dstArrayElement = arrayElement;
	write.descriptorType = type;
	write.descriptorCount = 1;
	write.pImageInfo = &imageInfo;

	vulkanDevice.device.updateDescriptorSets(write, {});
}

void DescriptorManager::update(DSetHandle dSet, uint32_t binding, uint32_t copyIndex, vk::DescriptorType type,
                               vk::Buffer buffer, vk::DeviceSize offset, vk::DeviceSize range)
{
	vk::DescriptorBufferInfo bufferInfo;
	bufferInfo.buffer = buffer;
	bufferInfo.offset = offset;
	bufferInfo.range = range;

	vk::WriteDescriptorSet write;
	write.dstSet = descriptorSets[dSet.id][copyIndex];
	write.dstBinding = binding;
	write.dstArrayElement = 0;
	write.descriptorType = type;
	write.descriptorCount = 1;
	write.pBufferInfo = &bufferInfo;

	vulkanDevice.device.updateDescriptorSets(write, {});
}

void DescriptorManager::updateSingleTextureDSet(DSetHandle dIndex, int binding, vk::ImageView imageView,
                                                vk::Sampler sampler)
{
	for (uint32_t i = 0; i < getSetCount(dIndex); ++i)
		update(dIndex, static_cast<uint32_t>(binding), i, vk::DescriptorType::eCombinedImageSampler, imageView, sampler);
}
