#include "GraphicsCore/Resources/Factories/TextureFactory.hpp"
#include "GraphicsCore/Resources/Managers/TextureManager.hpp"
#include "GraphicsCore/Resources/Managers/DescriptorManager.hpp"
#include "GraphicsCore/Resources/Factories/TextureUploader.hpp"
#include "GraphicsCore/Resources/Managers/Bindings.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

TextureHandle TextureFactory::createDepthImage(TextureManager& tManager, uint32_t width, uint32_t height)
{
	TextureHandle handle{tManager.allocateTextureSlot()};
	Texture& texture = tManager.getTexture(handle);
	vk::Format depthFormat = tManager.findBestFormat();

	ImageDesc desc;
	desc.width = width;
	desc.height = height;
	desc.format = depthFormat;
	desc.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled;
	tManager.createImage(texture, desc);
	tManager.createImageView(texture, depthFormat, vk::ImageAspectFlagBits::eDepth);
	tManager.createSampler(texture, samplerPresets::texture());
	return handle;
}

TextureHandle TextureFactory::createOffscreenImage(TextureManager& tManager, uint32_t width, uint32_t height,
                                                   vk::Format format)
{
	TextureHandle handle{tManager.allocateTextureSlot()};
	Texture& texture = tManager.getTexture(handle);

	ImageDesc desc;
	desc.width = width;
	desc.height = height;
	desc.format = format;
	desc.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled;
	tManager.createImage(texture, desc);
	tManager.createImageView(texture, format, vk::ImageAspectFlagBits::eColor);

	SamplerDesc samplerDesc;
	samplerDesc.addressMode = SamplerAddressMode::ClampToBorder;
	samplerDesc.borderColor = SamplerBorderColor::FloatOpaqueBlack;
	samplerDesc.compareOp = SamplerCompareOp::Greater;
	tManager.createSampler(texture, samplerDesc);
	return handle;
}

TextureHandle TextureFactory::createShadowMap(TextureManager& tManager, uint32_t width, uint32_t height)
{
	TextureHandle handle{tManager.allocateTextureSlot()};
	Texture& texture = tManager.getTexture(handle);
	vk::Format shadowFormat = tManager.findBestFormat();

	ImageDesc desc;
	desc.width = width;
	desc.height = height;
	desc.format = shadowFormat;
	desc.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled;
	tManager.createImage(texture, desc);
	tManager.createImageView(texture, shadowFormat, vk::ImageAspectFlagBits::eDepth);

	SamplerDesc samplerDesc;
	samplerDesc.addressMode = SamplerAddressMode::ClampToBorder;
	samplerDesc.borderColor = SamplerBorderColor::FloatOpaqueBlack;
	samplerDesc.compareOp = SamplerCompareOp::Greater;
	tManager.createSampler(texture, samplerDesc);
	return handle;
}

TextureHandle TextureFactory::generateTextureData(TextureManager& tManager, VulkanDevice& vulkanDevice,
                                                  VmaAllocator allocator, const char* texturePath, int texWidth,
                                                  int texHeight, const unsigned char* pixels,
                                                  BindlessTextureDSetComponent& dSetComponent,
                                                  DescriptorManager& dManager, vk::Format format)
{
	if (!pixels)
	{
		throw std::runtime_error("pixels data is null!");
	}
	if (texWidth <= 0 || texHeight <= 0)
	{
		throw std::runtime_error("Invalid texture dimensions!");
	}
	TextureHandle handle{tManager.allocateTextureSlot()};
	Texture& texture = tManager.getTexture(handle);

	uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
	ImageDesc desc;
	desc.width = texWidth;
	desc.height = texHeight;
	desc.format = format;
	desc.usage = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst |
	             vk::ImageUsageFlagBits::eSampled;
	desc.mipLevels = mipLevels;
	tManager.createImage(texture, desc);
	TextureUploader::uploadTextureFromBuffer(pixels, texWidth, texHeight, texture, allocator, vulkanDevice);
	tManager.createImageView(texture, format, vk::ImageAspectFlagBits::eColor);
	tManager.createSampler(texture, samplerPresets::texture());

	tManager.texturePaths[std::string(texturePath)] = handle;
	dManager.update(dSetComponent.bindlessTextureSet, Bindings::Textures::Array, 0,
	                vk::DescriptorType::eCombinedImageSampler, texture.textureImageView,
	                tManager.getSampler(texture.samplerHandle), vk::ImageLayout::eShaderReadOnlyOptimal,
	                static_cast<uint32_t>(handle.id));
	return handle;
}

TextureHandle TextureFactory::generateTextureDataFromKtx(TextureManager& tManager, VulkanDevice& vulkanDevice,
                                                         VmaAllocator allocator, const char* texturePath,
                                                         const unsigned char* ktxData, size_t dataSize,
                                                         BindlessTextureDSetComponent& dSetComponent,
                                                         DescriptorManager& dManager, bool isSrgb)
{
	TextureHandle handle{tManager.allocateTextureSlot()};
	Texture& texture = tManager.getTexture(handle);

	TextureUploader::uploadKtxTextureData(ktxData, dataSize, texture, tManager, isSrgb, allocator, vulkanDevice);
	tManager.createImageView(texture, texture.format, vk::ImageAspectFlagBits::eColor);
	tManager.createSampler(texture, samplerPresets::texture()); // maxLod uses texture.mipLevels set inside upload

	tManager.texturePaths[std::string(texturePath)] = handle;
	dManager.update(dSetComponent.bindlessTextureSet, Bindings::Textures::Array, 0,
	                vk::DescriptorType::eCombinedImageSampler, texture.textureImageView,
	                tManager.getSampler(texture.samplerHandle), vk::ImageLayout::eShaderReadOnlyOptimal,
	                static_cast<uint32_t>(handle.id));
	return handle;
}
