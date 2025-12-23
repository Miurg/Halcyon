#include "DescriptorHandlerFactory.hpp"

void DescriptorHandlerFactory::createDescriptorSetLayout(VulkanDevice& vulkanDevice,
                                                         DescriptorHandler& descriptorHandler)
{
	std::array bindings = {vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1,
	                                                      vk::ShaderStageFlagBits::eVertex, nullptr),
	                       vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, 1,
	                                                      vk::ShaderStageFlagBits::eFragment, nullptr)};
	vk::DescriptorSetLayoutCreateInfo layoutInfo({}, bindings.size(), bindings.data());
	descriptorHandler.descriptorSetLayout = vk::raii::DescriptorSetLayout(vulkanDevice.device, layoutInfo);
}

void DescriptorHandlerFactory::createDescriptorPool(VulkanDevice& vulkanDevice, DescriptorHandler& descriptorHandler) 
{
	std::array poolSize{vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1000 * MAX_FRAMES_IN_FLIGHT),
	                    vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1000 * MAX_FRAMES_IN_FLIGHT)};
	vk::DescriptorPoolCreateInfo poolInfo;
	poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	poolInfo.maxSets = 1000 * 2 * MAX_FRAMES_IN_FLIGHT;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSize.size());
	poolInfo.pPoolSizes = poolSize.data();

	descriptorHandler.descriptorPool = vk::raii::DescriptorPool(vulkanDevice.device, poolInfo);
}

void DescriptorHandlerFactory::allocateDescriptorSets(VulkanDevice& vulkanDevice, DescriptorHandler& descriptorHandler,
                                                      GameObject& gameObject)
{
	std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorHandler.descriptorSetLayout);

	vk::DescriptorSetAllocateInfo allocInfo;
	allocInfo.descriptorPool = descriptorHandler.descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
	allocInfo.pSetLayouts = layouts.data();

	gameObject.descriptorSets.clear();
	gameObject.descriptorSets = vulkanDevice.device.allocateDescriptorSets(allocInfo);
}

void DescriptorHandlerFactory::updateUniformDescriptors(VulkanDevice& vulkanDevice, GameObject& gameObject)
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vk::DescriptorBufferInfo bufferInfo;
		bufferInfo.buffer = *gameObject.uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		vk::WriteDescriptorSet descriptorWrite;
		descriptorWrite.dstSet = *gameObject.descriptorSets[i]; 
		descriptorWrite.dstBinding = 0;                         
		descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;

		vulkanDevice.device.updateDescriptorSets(descriptorWrite, {});
	}
}

void DescriptorHandlerFactory::updateTextureDescriptors(VulkanDevice& vulkanDevice, GameObject& gameObject)
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vk::DescriptorImageInfo imageInfo;
		imageInfo.sampler = gameObject.texture->textureSampler;
		imageInfo.imageView = gameObject.texture->textureImageView;
		imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

		vk::WriteDescriptorSet descriptorWrite;
		descriptorWrite.dstSet = *gameObject.descriptorSets[i];
		descriptorWrite.dstBinding = 1;                        
		descriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pImageInfo = &imageInfo;

		vulkanDevice.device.updateDescriptorSets(descriptorWrite, {});
	}
}