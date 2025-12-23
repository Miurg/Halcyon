#pragma once

#include <string>
#include "VulkanDevice.hpp"
#include <vector>
#include <vulkan/vulkan_raii.hpp>
#include "GameObject.hpp"

class MaterialAsset
{
public:
	static void generateTextureData(const std::string texturePath, VulkanDevice& vulkanDevice, GameObject& gameObject);

private:
	static void createTextureImage(const std::string texturePath, VulkanDevice& vulkanDevice, GameObject& gameObject);
	static void createTextureImageView(VulkanDevice& vulkanDevice, GameObject& gameObject);
	static void createTextureSampler(VulkanDevice& vulkanDevice, GameObject& gameObject);
	// Подписи синхронизированы с реализацией в MaterialAsset.cpp
	static void transitionImageLayout(const vk::raii::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, VulkanDevice& vulkanDevice);
	static void copyBufferToImage(const vk::raii::Buffer& buffer, const vk::raii::Image& image, uint32_t width,
	                             uint32_t height, VulkanDevice& vulkanDevice);
	
};