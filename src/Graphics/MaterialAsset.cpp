#include "MaterialAsset.hpp"
#include "VulkanUtils.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "GameObject.hpp"

void MaterialAsset::generateTextureData(const std::string texturePath, VulkanDevice& vulkanDevice, GameObject& gameObject)
{
	MaterialAsset::createTextureImage(texturePath, vulkanDevice, gameObject);
	MaterialAsset::createTextureImageView(vulkanDevice, gameObject);
	MaterialAsset::createTextureSampler(vulkanDevice, gameObject);
}

void MaterialAsset::createTextureImage(const std::string texturePath, VulkanDevice& vulkanDevice,
                                       GameObject& gameObject)
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	vk::DeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels)
	{
		throw std::runtime_error("failed to load texture image!");
	}
	auto result = VulkanUtils::createBuffer(
	    imageSize, vk::BufferUsageFlagBits::eTransferSrc,
	    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, vulkanDevice);
	vk::raii::Buffer stagingTextureBuffer = std::move(result.first);
	vk::raii::DeviceMemory stagingTextureBufferMemory = std::move(result.second);

	void* data = stagingTextureBufferMemory.mapMemory(0, imageSize);
	memcpy(data, pixels, imageSize);
	stagingTextureBufferMemory.unmapMemory();

	stbi_image_free(pixels);

	VulkanUtils::createImage(texWidth, texHeight, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal,
	                         vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
	                         vk::MemoryPropertyFlagBits::eDeviceLocal, gameObject.texture->textureImage,
	                         gameObject.texture->textureImageMemory,
	                         vulkanDevice);

	transitionImageLayout(gameObject.texture->textureImage, vk::ImageLayout::eUndefined,
	                      vk::ImageLayout::eTransferDstOptimal, vulkanDevice);
	copyBufferToImage(stagingTextureBuffer, gameObject.texture->textureImage, static_cast<uint32_t>(texWidth),
	                  static_cast<uint32_t>(texHeight), vulkanDevice);
	transitionImageLayout(gameObject.texture->textureImage, vk::ImageLayout::eTransferDstOptimal,
	                      vk::ImageLayout::eShaderReadOnlyOptimal,
	                      vulkanDevice);
}

void MaterialAsset::createTextureImageView(VulkanDevice& vulkanDevice, GameObject& gameObject)
{
	gameObject.texture->textureImageView = VulkanUtils::createImageView(gameObject.texture->textureImage, vk::Format::eR8G8B8A8Srgb,
	                                                vk::ImageAspectFlagBits::eColor, vulkanDevice);
}

void MaterialAsset::createTextureSampler(VulkanDevice& vulkanDevice, GameObject& gameObject)
{
	vk::PhysicalDeviceProperties properties = vulkanDevice.physicalDevice.getProperties();
	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.magFilter = vk::Filter::eLinear;
	samplerInfo.minFilter = vk::Filter::eLinear;
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
	samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
	samplerInfo.anisotropyEnable = vk::True;
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.compareEnable = vk::False;
	samplerInfo.compareOp = vk::CompareOp::eAlways;
	samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
	samplerInfo.unnormalizedCoordinates = vk::False;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;
	gameObject.texture->textureSampler = vk::raii::Sampler(vulkanDevice.device, samplerInfo);
}

void MaterialAsset::transitionImageLayout(const vk::raii::Image& image, vk::ImageLayout oldLayout,
                                          vk::ImageLayout newLayout, VulkanDevice& vulkanDevice)
{
	auto commandBuffer = VulkanUtils::beginSingleTimeCommands(vulkanDevice);

	vk::ImageMemoryBarrier barrier;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.image = image;
	barrier.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

	vk::PipelineStageFlags sourceStage;
	vk::PipelineStageFlags destinationStage;
	if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
	{
		barrier.srcAccessMask = {};
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		sourceStage = vk::PipelineStageFlagBits::eTransfer;
		destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else
	{
		throw std::invalid_argument("unsupported layout transition!");
	}
	commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, {}, barrier);

	VulkanUtils::endSingleTimeCommands(commandBuffer, vulkanDevice);
}

void MaterialAsset::copyBufferToImage(const vk::raii::Buffer& buffer, const vk::raii::Image& image, uint32_t width,
                                      uint32_t height, VulkanDevice& vulkanDevice)
{
	vk::raii::CommandBuffer commandBuffer = VulkanUtils::beginSingleTimeCommands(vulkanDevice);

	vk::BufferImageCopy region;
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1};
	region.imageOffset = vk::Offset3D{0, 0, 0};
	region.imageExtent = vk::Extent3D{width, height, 1};

	commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, {region});

	VulkanUtils::endSingleTimeCommands(commandBuffer, vulkanDevice);
}