#include "DescriptorHandlerFactory.hpp"

void DescriptorHandlerFactory::createDescriptorSetLayouts(VulkanDevice& vulkanDevice,
                                                          DescriptorHandler& descriptorHandler)
{
	//===Camera===
	vk::DescriptorSetLayoutBinding cameraBinding(0, vk::DescriptorType::eUniformBuffer, 1,
	                                             vk::ShaderStageFlagBits::eVertex, nullptr);
	vk::DescriptorSetLayoutCreateInfo cameraInfo({}, 1, &cameraBinding);
	descriptorHandler.cameraSetLayout = vk::raii::DescriptorSetLayout(vulkanDevice.device, cameraInfo);

	//===Textures===
	vk::DescriptorSetLayoutBinding textureBinding(0, vk::DescriptorType::eCombinedImageSampler, 1,
	                                              vk::ShaderStageFlagBits::eFragment, nullptr);
	vk::DescriptorSetLayoutCreateInfo textureInfo({}, 1, &textureBinding);
	descriptorHandler.textureSetLayout = vk::raii::DescriptorSetLayout(vulkanDevice.device, textureInfo);

	//===Model===
	vk::DescriptorSetLayoutBinding modelBinding(0, vk::DescriptorType::eStorageBuffer, 1,
	                                            vk::ShaderStageFlagBits::eVertex, nullptr);
	vk::DescriptorSetLayoutCreateInfo modelInfo({}, 1, &modelBinding);
	descriptorHandler.modelSetLayout = vk::raii::DescriptorSetLayout(vulkanDevice.device, modelInfo);
}

void DescriptorHandlerFactory::createDescriptorPool(VulkanDevice& vulkanDevice, DescriptorHandler& descriptorHandler)
{
	std::array poolSize{vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1000 * MAX_FRAMES_IN_FLIGHT),
	                    vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1000 * MAX_FRAMES_IN_FLIGHT),
	                    vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1000 * MAX_FRAMES_IN_FLIGHT)};
	vk::DescriptorPoolCreateInfo poolInfo;
	poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	poolInfo.maxSets = 1000 * 3 * MAX_FRAMES_IN_FLIGHT;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSize.size());
	poolInfo.pPoolSizes = poolSize.data();

	descriptorHandler.descriptorPool = vk::raii::DescriptorPool(vulkanDevice.device, poolInfo);
}

void DescriptorHandlerFactory::allocateGlobalDescriptorSets(VulkanDevice& vulkanDevice,
                                                            DescriptorHandler& descriptorHandler,
                                                            CameraComponent& camera)
{
	std::vector<vk::DescriptorSetLayout> cameraLayouts(MAX_FRAMES_IN_FLIGHT, *descriptorHandler.cameraSetLayout);

	vk::DescriptorSetAllocateInfo allocInfo;
	allocInfo.descriptorPool = *descriptorHandler.descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(cameraLayouts.size());
	allocInfo.pSetLayouts = cameraLayouts.data();

	camera.cameraDescriptorSets = vulkanDevice.device.allocateDescriptorSets(allocInfo);
}

void DescriptorHandlerFactory::allocateModelSSBOsDescriptors(VulkanDevice& vulkanDevice,
															  DescriptorHandler& descriptorHandler,
															  ModelSSBOsComponent& ssbos)
{
	std::vector<vk::DescriptorSetLayout> ssboLayouts(MAX_FRAMES_IN_FLIGHT, *descriptorHandler.modelSetLayout);
	vk::DescriptorSetAllocateInfo allocInfo;
	allocInfo.descriptorPool = *descriptorHandler.descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(ssboLayouts.size());
	allocInfo.pSetLayouts = ssboLayouts.data();
	ssbos.ssbos->modelSSBOsDescriptorSets = vulkanDevice.device.allocateDescriptorSets(allocInfo);
}

void DescriptorHandlerFactory::allocateObjectDescriptorSets(VulkanDevice& vulkanDevice,
                                                            DescriptorHandler& descriptorHandler,
                                                            GameObject& gameObject)
{
	vk::DescriptorSetAllocateInfo texAllocInfo;
	texAllocInfo.descriptorPool = *descriptorHandler.descriptorPool;
	texAllocInfo.descriptorSetCount = 1;
	vk::DescriptorSetLayout texLayout = *descriptorHandler.textureSetLayout;
	texAllocInfo.pSetLayouts = &texLayout;

	std::vector<vk::raii::DescriptorSet> allocatedTexSets = vulkanDevice.device.allocateDescriptorSets(texAllocInfo);
	gameObject.textureDescriptorSet = std::move(allocatedTexSets[0]);
}

void DescriptorHandlerFactory::updateCameraDescriptors(VulkanDevice& vulkanDevice, DescriptorHandler& descriptorHandler,
                                                       const CameraComponent& camera)
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vk::DescriptorBufferInfo bufferInfo;
		bufferInfo.buffer = *camera.cameraBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(CameraUBO);

		vk::WriteDescriptorSet descriptorWrite;
		descriptorWrite.dstSet = *camera.cameraDescriptorSets[i];
		descriptorWrite.dstBinding = 0; // Set 0, Binding 0
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;

		vulkanDevice.device.updateDescriptorSets(descriptorWrite, {});
	}
}

void DescriptorHandlerFactory::updateModelSSBOsDescriptors(VulkanDevice& vulkanDevice, ModelSSBOsComponent& ssbos)
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vk::DescriptorBufferInfo bufferInfo;
		bufferInfo.buffer = *ssbos.ssbos->modelSSBOs[i];
		bufferInfo.offset = 0;
		bufferInfo.range = VK_WHOLE_SIZE;

		vk::WriteDescriptorSet descriptorWrite;
		descriptorWrite.dstSet = *ssbos.ssbos->modelSSBOsDescriptorSets[i];
		descriptorWrite.dstBinding = 0; // Set 2, Binding 0
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;

		vulkanDevice.device.updateDescriptorSets(descriptorWrite, {});
	}
}

void DescriptorHandlerFactory::updateTextureDescriptors(VulkanDevice& vulkanDevice, GameObject& gameObject,
                                                        TextureInfoComponent& info, AssetManager& assetManager)
{
	vk::DescriptorImageInfo imageInfo;
	imageInfo.sampler = *assetManager.textures[info.textureIndex].textureSampler;
	imageInfo.imageView = *assetManager.textures[info.textureIndex].textureImageView;
	imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

	vk::WriteDescriptorSet descriptorWrite;
	descriptorWrite.dstSet = *gameObject.textureDescriptorSet;
	descriptorWrite.dstBinding = 0; // Set 1, Binding 0
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;

	vulkanDevice.device.updateDescriptorSets(descriptorWrite, {});
}