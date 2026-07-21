#include "GraphicsCore/Resources/Factories/EnvMapFactory.hpp"

#include "GraphicsCore/Resources/Managers/TextureManager.hpp"
#include "GraphicsCore/Resources/Managers/DescriptorManager.hpp"
#include "GraphicsCore/Resources/Managers/Bindings.hpp"
#include "GraphicsCore/Resources/Components/BindlessTextureDSetComponent.hpp"
#include "GraphicsCore/Managers/PipelineManager.hpp"
#include "GraphicsCore/Passes/PassCommands.hpp"
#include "GraphicsCore/VulkanDevice.hpp"
#include "GraphicsCore/VulkanUtils.hpp"
#include <algorithm>

TextureHandle EnvMapFactory::cubemapFromHdr(TextureManager& textureManager, VulkanDevice& vulkanDevice,
                                            TextureHandle hdrTexture, DescriptorManager& descriptorManager,
                                            BindlessTextureDSetComponent& dSetComponent,
                                            PipelineManager& pipelineManager)
{
	TextureHandle cubemapHandle{textureManager.allocateTextureSlot()};
	Texture& cubemapTexture = textureManager.getTexture(cubemapHandle);
	textureManager.createImage(cubemapTexture, imagePresets::cubemap(1024, vk::Format::eR32G32B32A32Sfloat,
	                                                                 vk::ImageUsageFlagBits::eSampled |
	                                                                     vk::ImageUsageFlagBits::eTransferDst |
	                                                                     vk::ImageUsageFlagBits::eStorage));
	textureManager.createImageView(cubemapTexture, vk::Format::eR32G32B32A32Sfloat, vk::ImageAspectFlagBits::eColor,
	                               vk::ImageViewType::eCube);
	textureManager.createSampler(cubemapTexture, samplerPresets::cubemap());

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

	descriptorManager.update(dSetComponent.bindlessTextureSet, Bindings::Textures::CubemapSampler, 0,
	                         vk::DescriptorType::eCombinedImageSampler, cubemapTexture.textureImageView,
	                         textureManager.getSampler(cubemapTexture.samplerHandle));
	descriptorManager.update(dSetComponent.bindlessTextureSet, Bindings::Textures::CubemapStorage, 0,
	                         vk::DescriptorType::eStorageImage, *storageImageView, nullptr, vk::ImageLayout::eGeneral);

	auto cmd = VulkanUtils::beginSingleTimeCommands(vulkanDevice);

	VulkanUtils::transitionImageLayout(
	    cmd, cubemapTexture.textureImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, {},
	    vk::AccessFlagBits2::eShaderWrite, vk::PipelineStageFlagBits2::eTopOfPipe,
	    vk::PipelineStageFlagBits2::eComputeShader, vk::ImageAspectFlagBits::eColor, 6, 1);

	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pipelineManager.pipelines["equirect_to_cube"].pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pipelineManager.pipelines["equirect_to_cube"].layout, 0,
	                       descriptorManager.getSet(dSetComponent.bindlessTextureSet), nullptr);

	uint32_t pushConstants = hdrTexture.id;
	cmd.pushConstants<uint32_t>(*pipelineManager.pipelines["equirect_to_cube"].layout, vk::ShaderStageFlagBits::eCompute,
	                            0, pushConstants);

	cmd.dispatch(1024 / 8, 1024 / 8, 6); // TODO: Get rid of hardcoded resolution. Same in SH compute.

	VulkanUtils::transitionImageLayout(
	    cmd, cubemapTexture.textureImage, vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal,
	    vk::AccessFlagBits2::eShaderWrite, vk::AccessFlagBits2::eShaderRead, vk::PipelineStageFlagBits2::eComputeShader,
	    vk::PipelineStageFlagBits2::eFragmentShader, vk::ImageAspectFlagBits::eColor, 6, 1);

	VulkanUtils::endSingleTimeCommands(cmd, vulkanDevice);

	return cubemapHandle;
}

TextureHandle EnvMapFactory::prefilteredEnvMap(TextureManager& textureManager, VulkanDevice& vulkanDevice,
                                               TextureHandle envCubemap, DescriptorManager& descriptorManager,
                                               BindlessTextureDSetComponent& dSetComponent,
                                               PipelineManager& pipelineManager)
{
	const uint32_t prefilteredSize = 128;
	const uint32_t maxMipLevels = 5;

	TextureHandle prefilteredHandle{textureManager.allocateTextureSlot()};
	Texture& prefilteredTexture = textureManager.getTexture(prefilteredHandle);
	textureManager.createImage(prefilteredTexture,
	                           imagePresets::cubemap(prefilteredSize, vk::Format::eR32G32B32A32Sfloat,
	                                                 vk::ImageUsageFlagBits::eSampled |
	                                                     vk::ImageUsageFlagBits::eTransferDst |
	                                                     vk::ImageUsageFlagBits::eStorage,
	                                                 maxMipLevels));
	textureManager.createImageView(prefilteredTexture, vk::Format::eR32G32B32A32Sfloat, vk::ImageAspectFlagBits::eColor,
	                               vk::ImageViewType::eCube);
	textureManager.createSampler(prefilteredTexture, samplerPresets::cubemap());

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

		descriptorManager.update(dSetComponent.bindlessTextureSet, Bindings::Textures::CubemapSampler, 0,
		                         vk::DescriptorType::eCombinedImageSampler,
		                         textureManager.getTexture(envCubemap).textureImageView,
		                         textureManager.getSampler(textureManager.getTexture(envCubemap).samplerHandle));
		descriptorManager.update(dSetComponent.bindlessTextureSet, Bindings::Textures::CubemapStorage, 0,
		                         vk::DescriptorType::eStorageImage, *mipStorageView, nullptr, vk::ImageLayout::eGeneral);

		auto cmd = VulkanUtils::beginSingleTimeCommands(vulkanDevice);

		cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pipelineManager.pipelines["prefilter_env_map"].pipeline);
		cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pipelineManager.pipelines["prefilter_env_map"].layout, 0,
		                       descriptorManager.getSet(dSetComponent.bindlessTextureSet), nullptr);

		float roughness = static_cast<float>(mip) / static_cast<float>(maxMipLevels - 1);
		cmd.pushConstants<float>(*pipelineManager.pipelines["prefilter_env_map"].layout,
		                         vk::ShaderStageFlagBits::eCompute, 0, roughness);

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

TextureHandle EnvMapFactory::brdfLut(TextureManager& textureManager, VulkanDevice& vulkanDevice,
                                     DescriptorManager& descriptorManager, BindlessTextureDSetComponent& dSetComponent,
                                     PipelineManager& pipelineManager)
{
	const uint32_t brdfLutSize = 512;

	int slot = textureManager.allocateTextureSlot();
	Texture& brdfLutTexture = textureManager.getTexture(TextureHandle{slot});

	ImageDesc brdfLutDesc;
	brdfLutDesc.width = brdfLutSize;
	brdfLutDesc.height = brdfLutSize;
	brdfLutDesc.format = vk::Format::eR32G32Sfloat;
	brdfLutDesc.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage;
	textureManager.createImage(brdfLutTexture, brdfLutDesc);
	textureManager.createImageView(brdfLutTexture, vk::Format::eR32G32Sfloat, vk::ImageAspectFlagBits::eColor);

	SamplerDesc samplerDesc;
	samplerDesc.addressMode = SamplerAddressMode::ClampToEdge;
	textureManager.createSampler(brdfLutTexture, samplerDesc);

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
	storageWrite.dstSet = descriptorManager.getSet(dSetComponent.bindlessTextureSet);
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

	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pipelineManager.pipelines["brdf_lut"].pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pipelineManager.pipelines["brdf_lut"].layout, 0,
	                       descriptorManager.getSet(dSetComponent.bindlessTextureSet), nullptr);

	cmd.dispatch(brdfLutSize / 8, brdfLutSize / 8, 1);

	VulkanUtils::transitionImageLayout(
	    cmd, brdfLutTexture.textureImage, vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal,
	    vk::AccessFlagBits2::eShaderWrite, vk::AccessFlagBits2::eShaderRead, vk::PipelineStageFlagBits2::eComputeShader,
	    vk::PipelineStageFlagBits2::eFragmentShader, vk::ImageAspectFlagBits::eColor, 1, 1);

	VulkanUtils::endSingleTimeCommands(cmd, vulkanDevice);

	return TextureHandle{slot};
}

void EnvMapFactory::bakeSHForProbe(TextureManager& textureManager, VulkanDevice& vulkanDevice, TextureHandle envCubemap,
                                   int probeSlot, DescriptorManager& descriptorManager,
                                   BindlessTextureDSetComponent& dSetComponent, DSetHandle globalDSet,
                                   PipelineManager& pipelineManager)
{
	Texture& envTex = textureManager.getTexture(envCubemap);
	descriptorManager.update(dSetComponent.bindlessTextureSet, Bindings::Textures::GICaptureCubemap, 0,
	                         vk::DescriptorType::eCombinedImageSampler, envTex.textureImageView,
	                         textureManager.getSampler(envTex.samplerHandle));

	auto cmd = VulkanUtils::beginSingleTimeCommands(vulkanDevice);
	recordSHProjection(cmd, static_cast<int>(envTex.width), probeSlot, descriptorManager, dSetComponent, globalDSet,
	                   pipelineManager);
	VulkanUtils::endSingleTimeCommands(cmd, vulkanDevice);
}
