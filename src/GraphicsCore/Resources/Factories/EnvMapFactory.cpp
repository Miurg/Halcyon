#include "GraphicsCore/Resources/Factories/EnvMapFactory.hpp"

#include "GraphicsCore/Resources/Managers/TextureManager.hpp"
#include "GraphicsCore/Resources/Managers/DescriptorManager.hpp"
#include "GraphicsCore/Resources/Managers/Bindings.hpp"
#include "GraphicsCore/Resources/Components/BindlessTextureDSetComponent.hpp"
#include "GraphicsCore/Managers/PipelineManager.hpp"
#include "GraphicsCore/VulkanDevice.hpp"
#include "GraphicsCore/VulkanUtils.hpp"
#include <algorithm>

TextureHandle EnvMapFactory::cubemapFromHdr(TextureManager& tManager, VulkanDevice& vulkanDevice,
                                            TextureHandle hdrTexture, DescriptorManager& dManager,
                                            BindlessTextureDSetComponent& dSetComponent, PipelineManager& pManager)
{
	TextureHandle cubemapHandle{tManager.allocateTextureSlot()};
	Texture& cubemapTexture = tManager.getTexture(cubemapHandle);
	tManager.createImage(cubemapTexture,
	                     imagePresets::cubemap(1024, vk::Format::eR32G32B32A32Sfloat,
	                                           vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst |
	                                               vk::ImageUsageFlagBits::eStorage));
	tManager.createImageView(cubemapTexture, vk::Format::eR32G32B32A32Sfloat, vk::ImageAspectFlagBits::eColor,
	                         vk::ImageViewType::eCube);
	tManager.createSampler(cubemapTexture, samplerPresets::cubemap());

	vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = cubemapTexture.textureImage;
	viewInfo.viewType = vk::ImageViewType::e2DArray;
	viewInfo.format = vk::Format::eR32G32B32A32Sfloat;
	viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 6;
	vk::raii::ImageView storageImageView(vulkanDevice.device, viewInfo);

	dManager.update(dSetComponent.bindlessTextureSet, Bindings::Textures::CubemapSampler, 0,
	                vk::DescriptorType::eCombinedImageSampler, cubemapTexture.textureImageView,
	                cubemapTexture.textureSampler);
	dManager.update(dSetComponent.bindlessTextureSet, Bindings::Textures::CubemapStorage, 0,
	                vk::DescriptorType::eStorageImage, *storageImageView, nullptr, vk::ImageLayout::eGeneral);

	auto cmd = VulkanUtils::beginSingleTimeCommands(vulkanDevice);

	VulkanUtils::transitionImageLayout(
	    cmd, cubemapTexture.textureImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, {},
	    vk::AccessFlagBits2::eShaderWrite, vk::PipelineStageFlagBits2::eTopOfPipe,
	    vk::PipelineStageFlagBits2::eComputeShader, vk::ImageAspectFlagBits::eColor, 6, 1);

	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pManager.pipelines["equirect_to_cube"].pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pManager.pipelines["equirect_to_cube"].layout, 0,
	                       dManager.getSet(dSetComponent.bindlessTextureSet), nullptr);

	uint32_t pushConstants = hdrTexture.id;
	cmd.pushConstants<uint32_t>(*pManager.pipelines["equirect_to_cube"].layout, vk::ShaderStageFlagBits::eCompute, 0,
	                            pushConstants);

	cmd.dispatch(1024 / 8, 1024 / 8, 6); // TODO: Get rid of hardcoded resolution. Same in SH compute.

	VulkanUtils::transitionImageLayout(
	    cmd, cubemapTexture.textureImage, vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal,
	    vk::AccessFlagBits2::eShaderWrite, vk::AccessFlagBits2::eShaderRead, vk::PipelineStageFlagBits2::eComputeShader,
	    vk::PipelineStageFlagBits2::eFragmentShader, vk::ImageAspectFlagBits::eColor, 6, 1);

	VulkanUtils::endSingleTimeCommands(cmd, vulkanDevice);

	return cubemapHandle;
}

TextureHandle EnvMapFactory::prefilteredEnvMap(TextureManager& tManager, VulkanDevice& vulkanDevice,
                                               TextureHandle envCubemap, DescriptorManager& dManager,
                                               BindlessTextureDSetComponent& dSetComponent, PipelineManager& pManager)
{
	const uint32_t prefilteredSize = 128;
	const uint32_t maxMipLevels = 5;

	TextureHandle prefilteredHandle{tManager.allocateTextureSlot()};
	Texture& prefilteredTexture = tManager.getTexture(prefilteredHandle);
	tManager.createImage(prefilteredTexture,
	                     imagePresets::cubemap(prefilteredSize, vk::Format::eR32G32B32A32Sfloat,
	                                           vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst |
	                                               vk::ImageUsageFlagBits::eStorage,
	                                           maxMipLevels));
	tManager.createImageView(prefilteredTexture, vk::Format::eR32G32B32A32Sfloat, vk::ImageAspectFlagBits::eColor,
	                         vk::ImageViewType::eCube);
	tManager.createSampler(prefilteredTexture, samplerPresets::cubemap());

	auto initCmd = VulkanUtils::beginSingleTimeCommands(vulkanDevice);

	VulkanUtils::transitionImageLayout(
	    initCmd, prefilteredTexture.textureImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, {},
	    vk::AccessFlagBits2::eShaderWrite, vk::PipelineStageFlagBits2::eTopOfPipe,
	    vk::PipelineStageFlagBits2::eComputeShader, vk::ImageAspectFlagBits::eColor, 6, maxMipLevels);

	VulkanUtils::endSingleTimeCommands(initCmd, vulkanDevice);

	// Process each mip level in a separate command buffer so storage views stay alive
	for (uint32_t mip = 0; mip < maxMipLevels; mip++)
	{
		uint32_t mipWidth = prefilteredSize >> mip;
		uint32_t mipHeight = prefilteredSize >> mip;

		vk::ImageViewCreateInfo viewInfo;
		viewInfo.image = prefilteredTexture.textureImage;
		viewInfo.viewType = vk::ImageViewType::e2DArray;
		viewInfo.format = vk::Format::eR32G32B32A32Sfloat;
		viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		viewInfo.subresourceRange.baseMipLevel = mip;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 6;
		vk::raii::ImageView mipStorageView(vulkanDevice.device, viewInfo);

		dManager.update(dSetComponent.bindlessTextureSet, Bindings::Textures::CubemapSampler, 0,
		                vk::DescriptorType::eCombinedImageSampler, tManager.getTexture(envCubemap).textureImageView,
		                tManager.getTexture(envCubemap).textureSampler);
		dManager.update(dSetComponent.bindlessTextureSet, Bindings::Textures::CubemapStorage, 0,
		                vk::DescriptorType::eStorageImage, *mipStorageView, nullptr, vk::ImageLayout::eGeneral);

		auto cmd = VulkanUtils::beginSingleTimeCommands(vulkanDevice);

		cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pManager.pipelines["prefilter_env_map"].pipeline);
		cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pManager.pipelines["prefilter_env_map"].layout, 0,
		                       dManager.getSet(dSetComponent.bindlessTextureSet), nullptr);

		float roughness = static_cast<float>(mip) / static_cast<float>(maxMipLevels - 1);
		cmd.pushConstants<float>(*pManager.pipelines["prefilter_env_map"].layout, vk::ShaderStageFlagBits::eCompute, 0,
		                         roughness);

		uint32_t groupsX = std::max(1u, mipWidth / 8);
		uint32_t groupsY = std::max(1u, mipHeight / 8);
		cmd.dispatch(groupsX, groupsY, 6);

		VulkanUtils::endSingleTimeCommands(cmd, vulkanDevice);
	}

	auto finalCmd = VulkanUtils::beginSingleTimeCommands(vulkanDevice);

	VulkanUtils::transitionImageLayout(
	    finalCmd, prefilteredTexture.textureImage, vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal,
	    vk::AccessFlagBits2::eShaderWrite, vk::AccessFlagBits2::eShaderRead, vk::PipelineStageFlagBits2::eComputeShader,
	    vk::PipelineStageFlagBits2::eFragmentShader, vk::ImageAspectFlagBits::eColor, 6, maxMipLevels);

	VulkanUtils::endSingleTimeCommands(finalCmd, vulkanDevice);

	return prefilteredHandle;
}

TextureHandle EnvMapFactory::brdfLut(TextureManager& tManager, VulkanDevice& vulkanDevice, DescriptorManager& dManager,
                                     BindlessTextureDSetComponent& dSetComponent, PipelineManager& pManager)
{
	const uint32_t brdfLutSize = 512;

	int slot = tManager.allocateTextureSlot();
	Texture& brdfLutTexture = tManager.getTexture(TextureHandle{slot});

	ImageDesc brdfLutDesc;
	brdfLutDesc.width = brdfLutSize;
	brdfLutDesc.height = brdfLutSize;
	brdfLutDesc.format = vk::Format::eR32G32Sfloat;
	brdfLutDesc.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage;
	tManager.createImage(brdfLutTexture, brdfLutDesc);
	tManager.createImageView(brdfLutTexture, vk::Format::eR32G32Sfloat, vk::ImageAspectFlagBits::eColor);

	SamplerDesc samplerDesc;
	samplerDesc.addressMode = SamplerAddressMode::ClampToEdge;
	tManager.createSampler(brdfLutTexture, samplerDesc);

	vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = brdfLutTexture.textureImage;
	viewInfo.viewType = vk::ImageViewType::e2D;
	viewInfo.format = vk::Format::eR32G32Sfloat;
	viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	vk::raii::ImageView storageView(vulkanDevice.device, viewInfo);

	vk::DescriptorImageInfo storageImgInfo;
	storageImgInfo.sampler = nullptr;
	storageImgInfo.imageView = *storageView;
	storageImgInfo.imageLayout = vk::ImageLayout::eGeneral;

	vk::WriteDescriptorSet storageWrite;
	storageWrite.dstSet = dManager.getSet(dSetComponent.bindlessTextureSet);
	storageWrite.dstBinding = Bindings::Textures::CubemapStorage;
	storageWrite.dstArrayElement = 0;
	storageWrite.descriptorType = vk::DescriptorType::eStorageImage;
	storageWrite.descriptorCount = 1;
	storageWrite.pImageInfo = &storageImgInfo;
	vulkanDevice.device.updateDescriptorSets(storageWrite, {});

	auto cmd = VulkanUtils::beginSingleTimeCommands(vulkanDevice);

	VulkanUtils::transitionImageLayout(
	    cmd, brdfLutTexture.textureImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, {},
	    vk::AccessFlagBits2::eShaderWrite, vk::PipelineStageFlagBits2::eTopOfPipe,
	    vk::PipelineStageFlagBits2::eComputeShader, vk::ImageAspectFlagBits::eColor, 1, 1);

	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pManager.pipelines["brdf_lut"].pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pManager.pipelines["brdf_lut"].layout, 0,
	                       dManager.getSet(dSetComponent.bindlessTextureSet), nullptr);

	cmd.dispatch(brdfLutSize / 8, brdfLutSize / 8, 1);

	VulkanUtils::transitionImageLayout(
	    cmd, brdfLutTexture.textureImage, vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal,
	    vk::AccessFlagBits2::eShaderWrite, vk::AccessFlagBits2::eShaderRead, vk::PipelineStageFlagBits2::eComputeShader,
	    vk::PipelineStageFlagBits2::eFragmentShader, vk::ImageAspectFlagBits::eColor, 1, 1);

	VulkanUtils::endSingleTimeCommands(cmd, vulkanDevice);

	return TextureHandle{slot};
}
