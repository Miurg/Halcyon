#pragma once

#include <vulkan/vulkan_raii.hpp>
#include "../VulkanDevice.hpp"
#include "../DescriptorHandler.hpp"
#include "../Resources/Managers/AssetManager.hpp"
#include "../Components/CameraComponent.hpp"

class DescriptorHandlerFactory
{
public:
	static void createDescriptorSetLayouts(VulkanDevice& vulkanDevice, DescriptorHandler& descriptorHandler);
	static void createDescriptorPool(VulkanDevice& vulkanDevice, DescriptorHandler& descriptorHandler);
	static void allocateGlobalDescriptorSets(VulkanDevice& vulkanDevice, DescriptorHandler& descriptorHandler,
	                                         CameraComponent& camera);
	static void allocateObjectDescriptorSets(VulkanDevice& vulkanDevice, DescriptorHandler& descriptorHandler,
	                                         GameObject& gameObject);
	static void updateCameraDescriptors(VulkanDevice& vulkanDevice, DescriptorHandler& descriptorHandler,
	                                    const CameraComponent& camera);
	static void updateModelDescriptors(VulkanDevice& vulkanDevice, GameObject& gameObject);
	static void updateTextureDescriptors(VulkanDevice& vulkanDevice, GameObject& gameObject, TextureInfoComponent& info,
	                                     AssetManager& assetManager);
};