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
		    vk::DescriptorSetLayoutBinding(Bindings::Textures::SHCoefficients, vk::DescriptorType::eStorageBuffer, 1,
		                                   S::eFragment | S::eCompute),
		    vk::DescriptorSetLayoutBinding(Bindings::Textures::PrefilteredMap, vk::DescriptorType::eCombinedImageSampler,
		                                   1, S::eFragment),
		    vk::DescriptorSetLayoutBinding(Bindings::Textures::BrdfLut, vk::DescriptorType::eCombinedImageSampler, 1,
		                                   S::eFragment),
		};
		std::array<vk::DescriptorBindingFlags, 8> textureBindingFlags = {
		    vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eUpdateAfterBind,
		    vk::DescriptorBindingFlags{}, // shadowMap
		    vk::DescriptorBindingFlags{}, // materials
		    vk::DescriptorBindingFlags{}, // cubemapSampler
		    vk::DescriptorBindingFlags{}, // cubemapStorage
		    vk::DescriptorBindingFlags{}, // shCoefficients
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
			ssBindings[i].stageFlags = vk::ShaderStageFlagBits::eFragment;
		}
		registerLayout("screenSpaceSet", ssBindings);
	}

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

DSetHandle DescriptorManager::allocateBindlessTextureDSet()
{
	vk::DescriptorSetLayout layout = layoutRegistry.get("textureSet");
	vk::DescriptorSetAllocateInfo allocInfo(*descriptorPool, layout);
	auto allocatedSets = vulkanDevice.device.allocateDescriptorSets(allocInfo);
	std::vector descriptors = {allocatedSets[0].release()};
	descriptorSets.push_back(descriptors);
	return DSetHandle{static_cast<int>(descriptorSets.size() - 1)};
}

DSetHandle DescriptorManager::allocateStorageBufferDSets(uint32_t count, const std::string& layoutName)
{
	vk::DescriptorSetLayout layout = layoutRegistry.get(layoutName);
	std::vector<vk::DescriptorSetLayout> layouts(count, layout);
	vk::DescriptorSetAllocateInfo allocInfo{};
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = count;
	allocInfo.pSetLayouts = layouts.data();

	auto allocatedSets = (*vulkanDevice.device).allocateDescriptorSets(allocInfo);
	descriptorSets.push_back(allocatedSets);
	return DSetHandle{static_cast<int>(descriptorSets.size() - 1)};
}

DSetHandle DescriptorManager::allocateOffscreenDescriptorSet(const std::string& layoutName)
{
	vk::DescriptorSetLayout layout = layoutRegistry.get(layoutName);
	vk::DescriptorSetAllocateInfo allocInfo(*descriptorPool, layout);
	auto allocatedSets = vulkanDevice.device.allocateDescriptorSets(allocInfo);

	std::vector<vk::DescriptorSet> descriptors = {allocatedSets[0].release()};
	descriptorSets.push_back(descriptors);

	return DSetHandle{static_cast<int>(descriptorSets.size() - 1)};
}

void DescriptorManager::updateBindlessTextureSet(vk::ImageView textureImageView, vk::Sampler textureSampler,
                                                 BindlessTextureDSetComponent& dSetComponent, int textureNumber)
{
	vk::DescriptorImageInfo imageInfo;
	imageInfo.sampler = textureSampler;
	imageInfo.imageView = textureImageView;
	imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

	vk::WriteDescriptorSet descriptorWrite;
	descriptorWrite.dstSet = descriptorSets[dSetComponent.bindlessTextureSet.id][0];
	descriptorWrite.dstBinding = Bindings::Textures::Array;
	descriptorWrite.dstArrayElement = textureNumber;
	descriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;

	vulkanDevice.device.updateDescriptorSets(descriptorWrite, {});
}

void DescriptorManager::updateCubemapDescriptors(BindlessTextureDSetComponent& dSetComponent,
                                                 vk::ImageView cubemapImageView, vk::Sampler cubemapSampler,
                                                 vk::ImageView storageImageView)
{
	vk::DescriptorImageInfo samplerInfo;
	samplerInfo.sampler = cubemapSampler;
	samplerInfo.imageView = cubemapImageView;
	samplerInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

	vk::DescriptorImageInfo storageInfo;
	storageInfo.sampler = nullptr;
	storageInfo.imageView = storageImageView;
	storageInfo.imageLayout = vk::ImageLayout::eGeneral;

	std::array<vk::WriteDescriptorSet, 2> descriptorWrites;

	descriptorWrites[0].dstSet = descriptorSets[dSetComponent.bindlessTextureSet.id][0];
	descriptorWrites[0].dstBinding = Bindings::Textures::CubemapSampler;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = vk::DescriptorType::eCombinedImageSampler;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pImageInfo = &samplerInfo;

	descriptorWrites[1].dstSet = descriptorSets[dSetComponent.bindlessTextureSet.id][0];
	descriptorWrites[1].dstBinding = Bindings::Textures::CubemapStorage;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = vk::DescriptorType::eStorageImage;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pImageInfo = &storageInfo;

	vulkanDevice.device.updateDescriptorSets(descriptorWrites, {});
}

void DescriptorManager::updateIBLDescriptors(BindlessTextureDSetComponent& dSetComponent, vk::ImageView prefilteredView,
                                             vk::Sampler prefilteredSampler, vk::ImageView brdfLutView,
                                             vk::Sampler brdfLutSampler)
{
	vk::DescriptorImageInfo prefilteredInfo;
	prefilteredInfo.sampler = prefilteredSampler;
	prefilteredInfo.imageView = prefilteredView;
	prefilteredInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

	vk::DescriptorImageInfo brdfLutInfo;
	brdfLutInfo.sampler = brdfLutSampler;
	brdfLutInfo.imageView = brdfLutView;
	brdfLutInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

	std::array<vk::WriteDescriptorSet, 2> descriptorWrites;

	descriptorWrites[0].dstSet = descriptorSets[dSetComponent.bindlessTextureSet.id][0];
	descriptorWrites[0].dstBinding = Bindings::Textures::PrefilteredMap;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = vk::DescriptorType::eCombinedImageSampler;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pImageInfo = &prefilteredInfo;

	descriptorWrites[1].dstSet = descriptorSets[dSetComponent.bindlessTextureSet.id][0];
	descriptorWrites[1].dstBinding = Bindings::Textures::BrdfLut;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pImageInfo = &brdfLutInfo;

	vulkanDevice.device.updateDescriptorSets(descriptorWrites, {});
}

void DescriptorManager::updateSHBufferDescriptor(BindlessTextureDSetComponent& dSetComponent, vk::Buffer shBuffer,
                                                 vk::DeviceSize bufferSize)
{
	vk::DescriptorBufferInfo bufferInfo;
	bufferInfo.buffer = shBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = bufferSize;

	vk::WriteDescriptorSet descriptorWrite;
	descriptorWrite.dstSet = descriptorSets[dSetComponent.bindlessTextureSet.id][0];
	descriptorWrite.dstBinding = Bindings::Textures::SHCoefficients;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pBufferInfo = &bufferInfo;

	vulkanDevice.device.updateDescriptorSets(descriptorWrite, {});
}

void DescriptorManager::updateSingleTextureDSet(DSetHandle dIndex, int binding, vk::ImageView imageView,
                                                vk::Sampler sampler)
{
	for (size_t i = 0; i < descriptorSets[dIndex.id].size(); i++)
	{
		vk::DescriptorImageInfo imageInfo;
		imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		imageInfo.imageView = imageView;
		imageInfo.sampler = sampler;

		vk::WriteDescriptorSet descriptorWrite;
		descriptorWrite.dstSet = descriptorSets[dIndex.id][i];
		descriptorWrite.dstBinding = binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pImageInfo = &imageInfo;

		vulkanDevice.device.updateDescriptorSets(descriptorWrite, {});
	}
}

void DescriptorManager::updateStorageBufferDescriptors(BufferManager& bManager, BufferHandle bNumber, DSetHandle dSet,
                                                       uint32_t binding)
{
	const size_t count = descriptorSets[dSet.id].size();
	assert(bManager.buffers[bNumber.id].buffer.size() >= count);

	std::vector<vk::WriteDescriptorSet> writes;
	writes.reserve(count);

	std::vector<vk::DescriptorBufferInfo> bufferInfos(count);

	for (size_t i = 0; i < count; ++i)
	{
		bufferInfos[i].buffer = bManager.buffers[bNumber.id].buffer[i];
		bufferInfos[i].offset = 0;
		bufferInfos[i].range = VK_WHOLE_SIZE;

		vk::WriteDescriptorSet write{};
		write.dstSet = descriptorSets[dSet.id][i];
		write.dstBinding = binding;
		write.dstArrayElement = 0;
		write.descriptorType = vk::DescriptorType::eStorageBuffer;
		write.descriptorCount = 1;
		write.pBufferInfo = &bufferInfos[i];

		writes.push_back(write);
	}

	(*vulkanDevice.device).updateDescriptorSets(writes, {});
}
