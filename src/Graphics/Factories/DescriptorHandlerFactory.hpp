#pragma once

#include <vulkan/vulkan_raii.hpp>
#include "../VulkanDevice.hpp"
#include "../DescriptorHandler.hpp"
#include "../Resources/Managers/AssetManager.hpp"
#include "../Components/CameraComponent.hpp"
#include "../Resources/Components/ModelSSBOsComponent.hpp"

class DescriptorHandlerFactory
{
public:
	static void createDescriptorSetLayouts(VulkanDevice& vulkanDevice, DescriptorHandler& descriptorHandler);
	static void createDescriptorPool(VulkanDevice& vulkanDevice, DescriptorHandler& descriptorHandler);
	static void allocateGlobalDescriptorSets(VulkanDevice& vulkanDevice, DescriptorHandler& descriptorHandler,
	                                         CameraComponent& camera);
	static void allocateModelSSBOsDescriptors(VulkanDevice& vulkanDevice, DescriptorHandler& descriptorHandler,
	                                          ModelSSBOsComponent& ssbos);
	static void allocateObjectDescriptorSets(VulkanDevice& vulkanDevice, DescriptorHandler& descriptorHandler,
	                                         GameObject& gameObject);
	static void updateCameraDescriptors(VulkanDevice& vulkanDevice, DescriptorHandler& descriptorHandler,
	                                    const CameraComponent& camera);
	static void updateModelSSBOsDescriptors(VulkanDevice& vulkanDevice, ModelSSBOsComponent& ssbos);
	static void updateTextureDescriptors(VulkanDevice& vulkanDevice, GameObject& gameObject, TextureInfoComponent& info,
	                                     AssetManager& assetManager);
};