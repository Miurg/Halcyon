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
	vk::DescriptorSetLayoutBinding cameraBinding(0, vk::DescriptorType::eStorageBuffer, 1,
	                                             vk::ShaderStageFlagBits::eVertex, nullptr);
	vk::DescriptorSetLayoutBinding shadowBinding(1, vk::DescriptorType::eCombinedImageSampler, 1,
	                                             vk::ShaderStageFlagBits::eFragment, nullptr);
	vk::DescriptorSetLayoutBinding sunBinding(2, vk::DescriptorType::eStorageBuffer, 1,
	                                          vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
	                                          nullptr);
	std::array<vk::DescriptorSetLayoutBinding, 3> globalBindings = {cameraBinding, shadowBinding, sunBinding};

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

int DescriptorManager::allocateBindlessTextureDSet()
{
	vk::DescriptorSetAllocateInfo allocInfo(*descriptorPool, *textureSetLayout);
	auto allocatedSets = vulkanDevice.device.allocateDescriptorSets(allocInfo);
	std::vector descriptors = {allocatedSets[0].release()};
	descriptorSets.push_back(descriptors);
	return descriptorSets.size() - 1;
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

void DescriptorManager::updateShadowDSet(int dIndex, vk::ImageView imageView, vk::Sampler sampler)
{
	for (size_t i = 0; i < descriptorSets[dIndex].size(); i++)
	{
		vk::DescriptorImageInfo imageInfo;
		imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		imageInfo.imageView = imageView;
		imageInfo.sampler = sampler;

		vk::WriteDescriptorSet descriptorWrite;
		descriptorWrite.dstSet = descriptorSets[dIndex][i];
		descriptorWrite.dstBinding = 1;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pImageInfo = &imageInfo;

		vulkanDevice.device.updateDescriptorSets(descriptorWrite, {});
	}
}

int DescriptorManager::allocateStorageBufferDSets(uint32_t count, vk::DescriptorSetLayout layout)
{
	std::vector<vk::DescriptorSetLayout> layouts(count, layout);
	vk::DescriptorSetAllocateInfo allocInfo{};
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = count;
	allocInfo.pSetLayouts = layouts.data();

	auto allocatedSets = (*vulkanDevice.device).allocateDescriptorSets(allocInfo);
	descriptorSets.push_back(allocatedSets);
	return descriptorSets.size() - 1;
}

void DescriptorManager::updateStorageBufferDescriptors(BufferManager& bManager, int bNumber,int dSet,uint32_t binding)
{
	const size_t count = descriptorSets[dSet].size();
	assert(bManager.buffers[bNumber].buffer.size() >= count);

	std::vector<vk::WriteDescriptorSet> writes;
	writes.reserve(count);

	std::vector<vk::DescriptorBufferInfo> bufferInfos(count);

	for (size_t i = 0; i < count; ++i)
	{
		bufferInfos[i].buffer = bManager.buffers[bNumber].buffer[i];
		bufferInfos[i].offset = 0;
		bufferInfos[i].range = VK_WHOLE_SIZE;

		vk::WriteDescriptorSet write{};
		write.dstSet = descriptorSets[dSet][i];
		write.dstBinding = binding;
		write.dstArrayElement = 0;
		write.descriptorType = vk::DescriptorType::eStorageBuffer;
		write.descriptorCount = 1;
		write.pBufferInfo = &bufferInfos[i];

		writes.push_back(write);
	}

	(*vulkanDevice.device).updateDescriptorSets(writes, {});
}