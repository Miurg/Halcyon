#include "DescriptorManager.hpp"
#include "BufferManager.hpp"

DescriptorManager::DescriptorManager(VulkanDevice& vulkanDevice) : vulkanDevice(vulkanDevice)
{
	std::array poolSize{vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1000 * MAX_FRAMES_IN_FLIGHT),
	                    vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler,
	                                           1000 * MAX_FRAMES_IN_FLIGHT + MAX_BINDLESS_TEXTURES),
	                    vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1000 * MAX_FRAMES_IN_FLIGHT)};

	vk::DescriptorPoolCreateInfo poolInfo;
	poolInfo.flags =
	    vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet | vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;
	poolInfo.maxSets = 1000 * 3 * MAX_FRAMES_IN_FLIGHT + 1;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSize.size());
	poolInfo.pPoolSizes = poolSize.data();

	descriptorPool = vk::raii::DescriptorPool(vulkanDevice.device, poolInfo);

	//===Global===
	vk::DescriptorSetLayoutBinding globalBinding(0, vk::DescriptorType::eStorageBuffer, 1,
	                                             vk::ShaderStageFlagBits::eVertex, nullptr);
	vk::DescriptorSetLayoutBinding shadowBinding(1, vk::DescriptorType::eCombinedImageSampler, 1,
	                                             vk::ShaderStageFlagBits::eFragment, nullptr);
	std::array<vk::DescriptorSetLayoutBinding, 2> globalBindings = {globalBinding, shadowBinding};

	vk::DescriptorSetLayoutCreateInfo globalInfo({}, static_cast<uint32_t>(globalBindings.size()),
	                                             globalBindings.data());
	globalSetLayout = vk::raii::DescriptorSetLayout(vulkanDevice.device, globalInfo);

	//===Textures===
	vk::DescriptorBindingFlags bindingFlags =
	    vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eUpdateAfterBind;
	vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo;
	bindingFlagsInfo.bindingCount = 1;
	bindingFlagsInfo.pBindingFlags = &bindingFlags;
	vk::DescriptorSetLayoutBinding textureBinding(0, vk::DescriptorType::eCombinedImageSampler, MAX_BINDLESS_TEXTURES,
	                                              vk::ShaderStageFlagBits::eFragment, nullptr);

	vk::DescriptorSetLayoutCreateInfo textureInfo;
	textureInfo.flags = vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;
	textureInfo.bindingCount = 1;
	textureInfo.pBindings = &textureBinding;
	textureInfo.pNext = &bindingFlagsInfo;
	textureSetLayout = vk::raii::DescriptorSetLayout(vulkanDevice.device, textureInfo);

	//===Model===
	vk::DescriptorSetLayoutBinding modelBinding(0, vk::DescriptorType::eStorageBuffer, 1,
	                                            vk::ShaderStageFlagBits::eVertex, nullptr);
	vk::DescriptorSetLayoutCreateInfo modelInfo({}, 1, &modelBinding);
	modelSetLayout = vk::raii::DescriptorSetLayout(vulkanDevice.device, modelInfo);
}

void DescriptorManager::allocateBindlessTextureDSet(BindlessTextureDSetComponent& dSetComponent)
{
	vk::DescriptorSetAllocateInfo allocInfo(*descriptorPool, *textureSetLayout);
	auto allocatedSets = vulkanDevice.device.allocateDescriptorSets(allocInfo);
	std::vector descriptors = {allocatedSets[0].release()};
	descriptorSets.push_back(descriptors);
	dSetComponent.bindlessTextureSet = descriptorSets.size()-1;
}

void DescriptorManager::updateBindlessTextureSet(int textureNumber, BindlessTextureDSetComponent& dSetComponent,
                                                 BufferManager& bManager)
{
	vk::DescriptorImageInfo imageInfo;
	imageInfo.sampler = bManager.textures[textureNumber].textureSampler;
	imageInfo.imageView = bManager.textures[textureNumber].textureImageView;
	imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

	vk::WriteDescriptorSet descriptorWrite;
	descriptorWrite.dstSet = descriptorSets[dSetComponent.bindlessTextureSet][0];
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = textureNumber;
	descriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;
	
	dSetComponent.bindlessTextureBuffer = textureNumber;

	vulkanDevice.device.updateDescriptorSets(descriptorWrite, {});
}

void DescriptorManager::updateShadowDSet(int bufferIndex, vk::ImageView imageView, vk::Sampler sampler,
                                  BufferManager& bManager)
{
	for (size_t i = 0; i < bManager.buffers[bufferIndex].descriptorSet.size(); i++)
	{
		vk::DescriptorImageInfo imageInfo;
		imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		imageInfo.imageView = imageView;
		imageInfo.sampler = sampler;

		vk::WriteDescriptorSet descriptorWrite;
		descriptorWrite.dstSet = bManager.buffers[bufferIndex].descriptorSet[i];
		descriptorWrite.dstBinding = 1;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pImageInfo = &imageInfo;

		vulkanDevice.device.updateDescriptorSets(descriptorWrite, {});
	}
}

void DescriptorManager::allocateStorageBufferDSets(Buffer& buffer, uint32_t count, vk::DescriptorSetLayout layout)
{
	buffer.descriptorSet.resize(count);

	std::vector<vk::DescriptorSetLayout> layouts(count, layout);

	vk::DescriptorSetAllocateInfo allocInfo{};
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = count;
	allocInfo.pSetLayouts = layouts.data();

	buffer.descriptorSet = (*vulkanDevice.device).allocateDescriptorSets(allocInfo);
}

void DescriptorManager::updateStorageBufferDescriptors(Buffer& buffer, uint32_t binding)
{
	const size_t count = buffer.descriptorSet.size();
	assert(buffer.buffer.size() >= count);

	std::vector<vk::WriteDescriptorSet> writes;
	writes.reserve(count);

	std::vector<vk::DescriptorBufferInfo> bufferInfos(count);

	for (size_t i = 0; i < count; ++i)
	{
		bufferInfos[i].buffer = buffer.buffer[i];
		bufferInfos[i].offset = 0;
		bufferInfos[i].range = VK_WHOLE_SIZE;

		vk::WriteDescriptorSet write{};
		write.dstSet = buffer.descriptorSet[i];
		write.dstBinding = binding;
		write.dstArrayElement = 0;
		write.descriptorType = vk::DescriptorType::eStorageBuffer;
		write.descriptorCount = 1;
		write.pBufferInfo = &bufferInfos[i];

		writes.push_back(write);
	}

	(*vulkanDevice.device).updateDescriptorSets(writes, {});
}