#include "DescriptorManager.hpp"
#include "BufferManager.hpp"
#include "Bindings.hpp"

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

	std::array imguiPoolSize{vk::DescriptorPoolSize(vk::DescriptorType::eSampler, 1000),
	                         vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1000),
	                         vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, 1000),
	                         vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 1000),
	                         vk::DescriptorPoolSize(vk::DescriptorType::eUniformTexelBuffer, 1000),
	                         vk::DescriptorPoolSize(vk::DescriptorType::eStorageTexelBuffer, 1000),
	                         vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1000),
	                         vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1000),
	                         vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1000),
	                         vk::DescriptorPoolSize(vk::DescriptorType::eStorageBufferDynamic, 1000),
	                         vk::DescriptorPoolSize(vk::DescriptorType::eInputAttachment, 1000)};

	vk::DescriptorPoolCreateInfo imguiPoolInfo;
	imguiPoolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	imguiPoolInfo.maxSets = 1000 * static_cast<uint32_t>(imguiPoolSize.size());
	imguiPoolInfo.poolSizeCount = static_cast<uint32_t>(imguiPoolSize.size());
	imguiPoolInfo.pPoolSizes = imguiPoolSize.data();

	imguiPool = vk::raii::DescriptorPool(vulkanDevice.device, imguiPoolInfo);

	// Set 0: Global
	{
		using S = vk::ShaderStageFlagBits;
		constexpr auto kAllStages = S::eVertex | S::eFragment | S::eCompute;
		std::array globalBindings = {
		    vk::DescriptorSetLayoutBinding(Bindings::Global::Camera, vk::DescriptorType::eStorageBuffer, 1, kAllStages),
		    vk::DescriptorSetLayoutBinding(Bindings::Global::Sun, vk::DescriptorType::eStorageBuffer, 1, kAllStages),
		    vk::DescriptorSetLayoutBinding(Bindings::Global::PointLights, vk::DescriptorType::eStorageBuffer, 1,
		                                   kAllStages),
		    vk::DescriptorSetLayoutBinding(Bindings::Global::PointLightCount, vk::DescriptorType::eStorageBuffer, 1,
		                                   kAllStages),
		    vk::DescriptorSetLayoutBinding(Bindings::Global::SHProbes, vk::DescriptorType::eStorageBuffer, 1, kAllStages),
		    vk::DescriptorSetLayoutBinding(Bindings::Global::SHProbeCount, vk::DescriptorType::eStorageBuffer, 1,
		                                   kAllStages),
		    vk::DescriptorSetLayoutBinding(Bindings::Global::GtaoTexture, vk::DescriptorType::eCombinedImageSampler, 1,
		                                   S::eFragment),
		};
		registerLayout("globalSet", globalBindings);
	}

	// Set 1: Model
	{
		using S = vk::ShaderStageFlagBits;
		std::array modelBindings = {
		    vk::DescriptorSetLayoutBinding(Bindings::Model::Primitives, vk::DescriptorType::eStorageBuffer, 1,
		                                   S::eVertex | S::eCompute),
		    vk::DescriptorSetLayoutBinding(Bindings::Model::Transforms, vk::DescriptorType::eStorageBuffer, 1,
		                                   S::eVertex | S::eCompute),
		    vk::DescriptorSetLayoutBinding(Bindings::Model::IndirectDraw, vk::DescriptorType::eStorageBuffer, 1,
		                                   S::eCompute),
		    vk::DescriptorSetLayoutBinding(Bindings::Model::VisibleIndices, vk::DescriptorType::eStorageBuffer, 1,
		                                   S::eCompute | S::eVertex),
		    vk::DescriptorSetLayoutBinding(Bindings::Model::CompactedDraw, vk::DescriptorType::eStorageBuffer, 1,
		                                   S::eCompute),
		    vk::DescriptorSetLayoutBinding(Bindings::Model::DrawCount, vk::DescriptorType::eStorageBuffer, 1,
		                                   S::eCompute | S::eVertex),
		};
		registerLayout("modelSet", modelBindings);
	}

	// Set 2: Textures
	{
		using S = vk::ShaderStageFlagBits;
		std::array textureBindings = {
		    vk::DescriptorSetLayoutBinding(Bindings::Textures::Array, vk::DescriptorType::eCombinedImageSampler,
		                                   MAX_BINDLESS_TEXTURES, S::eFragment | S::eCompute),
		    vk::DescriptorSetLayoutBinding(Bindings::Textures::ShadowMap, vk::DescriptorType::eCombinedImageSampler, 1,
		                                   S::eFragment),
		    vk::DescriptorSetLayoutBinding(Bindings::Textures::Materials, vk::DescriptorType::eStorageBuffer, 1,
		                                   S::eVertex | S::eFragment),
		    vk::DescriptorSetLayoutBinding(Bindings::Textures::CubemapSampler, vk::DescriptorType::eCombinedImageSampler,
		                                   1, S::eFragment | S::eCompute),
		    vk::DescriptorSetLayoutBinding(Bindings::Textures::CubemapStorage, vk::DescriptorType::eStorageImage, 1,
		                                   S::eCompute),
		    vk::DescriptorSetLayoutBinding(Bindings::Textures::PrefilteredMap, vk::DescriptorType::eCombinedImageSampler,
		                                   1, S::eFragment),
		    vk::DescriptorSetLayoutBinding(Bindings::Textures::BrdfLut, vk::DescriptorType::eCombinedImageSampler, 1,
		                                   S::eFragment),
		};
		std::array<vk::DescriptorBindingFlags, 7> textureBindingFlags = {
		    vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eUpdateAfterBind,
		    vk::DescriptorBindingFlags{}, // shadowMap
		    vk::DescriptorBindingFlags{}, // materials
		    vk::DescriptorBindingFlags{}, // cubemapSampler
		    vk::DescriptorBindingFlags{}, // cubemapStorage
		    vk::DescriptorBindingFlags{}, // prefilteredMap
		    vk::DescriptorBindingFlags{}, // brdfLut
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
	    vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, S::eCompute | S::eFragment | S::eVertex),
	    vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, S::eCompute),
	    vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, S::eCompute),
	    vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, 1, S::eCompute),
	    vk::DescriptorSetLayoutBinding(4, vk::DescriptorType::eStorageBuffer, 1, S::eCompute),
	    vk::DescriptorSetLayoutBinding(5, vk::DescriptorType::eStorageBuffer, 1, S::eCompute | S::eVertex),
	};
	registerLayout("particleSystemSet", particleSystemBindings);

} // Still mess.

DescriptorManager::~DescriptorManager()
{
	descriptorPool.reset();
	imguiPool.reset();
}

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

void DescriptorManager::updateBindlessTextureSet(vk::ImageView textureImageView, vk::Sampler textureSampler,
                                                 BindlessTextureDSetComponent& dSetComponent, int textureNumber)
{
	update(dSetComponent.bindlessTextureSet, Bindings::Textures::Array, 0, vk::DescriptorType::eCombinedImageSampler,
	       textureImageView, textureSampler, vk::ImageLayout::eShaderReadOnlyOptimal,
	       static_cast<uint32_t>(textureNumber));
}

void DescriptorManager::updateCubemapDescriptors(BindlessTextureDSetComponent& dSetComponent,
                                                 vk::ImageView cubemapImageView, vk::Sampler cubemapSampler,
                                                 vk::ImageView storageImageView)
{
	update(dSetComponent.bindlessTextureSet, Bindings::Textures::CubemapSampler, 0,
	       vk::DescriptorType::eCombinedImageSampler, cubemapImageView, cubemapSampler);
	update(dSetComponent.bindlessTextureSet, Bindings::Textures::CubemapStorage, 0, vk::DescriptorType::eStorageImage,
	       storageImageView, nullptr, vk::ImageLayout::eGeneral);
}

void DescriptorManager::updateIBLDescriptors(BindlessTextureDSetComponent& dSetComponent, vk::ImageView prefilteredView,
                                             vk::Sampler prefilteredSampler, vk::ImageView brdfLutView,
                                             vk::Sampler brdfLutSampler)
{
	update(dSetComponent.bindlessTextureSet, Bindings::Textures::PrefilteredMap, 0,
	       vk::DescriptorType::eCombinedImageSampler, prefilteredView, prefilteredSampler);
	update(dSetComponent.bindlessTextureSet, Bindings::Textures::BrdfLut, 0, vk::DescriptorType::eCombinedImageSampler,
	       brdfLutView, brdfLutSampler);
}

void DescriptorManager::updateSingleTextureDSet(DSetHandle dIndex, int binding, vk::ImageView imageView,
                                                vk::Sampler sampler)
{
	for (uint32_t i = 0; i < getSetCount(dIndex); ++i)
		update(dIndex, static_cast<uint32_t>(binding), i, vk::DescriptorType::eCombinedImageSampler, imageView, sampler);
}

void DescriptorManager::updateCubemapSamplerDescriptor(BindlessTextureDSetComponent& dSetComponent,
                                                       vk::ImageView cubemapImageView, vk::Sampler cubemapSampler)
{
	update(dSetComponent.bindlessTextureSet, Bindings::Textures::CubemapSampler, 0,
	       vk::DescriptorType::eCombinedImageSampler, cubemapImageView, cubemapSampler);
}

void DescriptorManager::updateStorageBufferDescriptors(BufferManager& bManager, BufferHandle bNumber, DSetHandle dSet,
                                                       uint32_t binding)
{
	const uint32_t copies = getSetCount(dSet);
	const auto& buffers = bManager.buffers[bNumber.id].buffer;
	const bool sharedBuf = (buffers.size() == 1);

	for (uint32_t i = 0; i < copies; ++i)
		update(dSet, binding, i, vk::DescriptorType::eStorageBuffer, buffers[sharedBuf ? 0 : i]);
}
