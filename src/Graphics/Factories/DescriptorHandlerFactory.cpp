#include "DescriptorHandlerFactory.hpp"

void DescriptorHandlerFactory::createDescriptorSetLayouts(VulkanDevice& vulkanDevice,
                                                          DescriptorHandler& descriptorHandler)
{
	// Layout для Uniform Buffer (Set 0)
	vk::DescriptorSetLayoutBinding uboBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex,
	                                          nullptr);
	vk::DescriptorSetLayoutCreateInfo uboLayoutInfo({}, 1, &uboBinding);
	descriptorHandler.uboSetLayout = vk::raii::DescriptorSetLayout(vulkanDevice.device, uboLayoutInfo);

	// Layout для Texture (Set 1)
	vk::DescriptorSetLayoutBinding samplerBinding(0, vk::DescriptorType::eCombinedImageSampler, 1,
	                                              vk::ShaderStageFlagBits::eFragment, nullptr);
	vk::DescriptorSetLayoutCreateInfo textureLayoutInfo({}, 1, &samplerBinding);
	descriptorHandler.textureSetLayout = vk::raii::DescriptorSetLayout(vulkanDevice.device, textureLayoutInfo);
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
	// Аллокация UBO сетов
	std::vector<vk::DescriptorSetLayout> uboLayouts(MAX_FRAMES_IN_FLIGHT, descriptorHandler.uboSetLayout);
	vk::DescriptorSetAllocateInfo uboAllocInfo;
	uboAllocInfo.descriptorPool = descriptorHandler.descriptorPool;
	uboAllocInfo.descriptorSetCount = static_cast<uint32_t>(uboLayouts.size());
	uboAllocInfo.pSetLayouts = uboLayouts.data();

	gameObject.uboDescriptorSets.clear(); // Предполагаем, что ты разделила хранение в GameObject
	gameObject.uboDescriptorSets = vulkanDevice.device.allocateDescriptorSets(uboAllocInfo);

	// Аллокация Texture сетов
	std::vector<vk::DescriptorSetLayout> texLayouts(MAX_FRAMES_IN_FLIGHT, descriptorHandler.textureSetLayout);
	vk::DescriptorSetAllocateInfo texAllocInfo;
	texAllocInfo.descriptorPool = descriptorHandler.descriptorPool;
	texAllocInfo.descriptorSetCount = static_cast<uint32_t>(texLayouts.size());
	texAllocInfo.pSetLayouts = texLayouts.data();

	gameObject.textureDescriptorSets.clear();
	gameObject.textureDescriptorSets = vulkanDevice.device.allocateDescriptorSets(texAllocInfo);
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
		descriptorWrite.dstSet = *gameObject.uboDescriptorSets[i]; 
		descriptorWrite.dstBinding = 0;                         
		descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;

		vulkanDevice.device.updateDescriptorSets(descriptorWrite, {});
	}
}

void DescriptorHandlerFactory::updateTextureDescriptors(VulkanDevice& vulkanDevice, GameObject& gameObject,
                                                        TextureInfoComponent& info,
                                                        AssetManager& assetManager)
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vk::DescriptorImageInfo imageInfo;
		imageInfo.sampler = assetManager.textures[info.textureIndex].textureSampler;
		imageInfo.imageView = assetManager.textures[info.textureIndex].textureImageView;
		imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

		vk::WriteDescriptorSet descriptorWrite;
		descriptorWrite.dstSet = *gameObject.textureDescriptorSets[i];
		descriptorWrite.dstBinding = 0;                        
		descriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pImageInfo = &imageInfo;

		vulkanDevice.device.updateDescriptorSets(descriptorWrite, {});
	}
}