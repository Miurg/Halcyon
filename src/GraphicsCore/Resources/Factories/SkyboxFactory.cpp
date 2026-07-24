#include "GraphicsCore/Resources/Factories/SkyboxFactory.hpp"

#include "GraphicsCore/Resources/Managers/TextureManager.hpp"
#include "GraphicsCore/Resources/Managers/DescriptorManager.hpp"
#include "GraphicsCore/Resources/Managers/Bindings.hpp"
#include "GraphicsCore/Resources/Factories/TextureUploader.hpp"
#include "GraphicsCore/Resources/Factories/EnvMapFactory.hpp"
#include "GraphicsCore/Components/TextureManagerComponent.hpp"
#include "GraphicsCore/Components/DescriptorManagerComponent.hpp"
#include "GraphicsCore/Components/VMAllocatorComponent.hpp"
#include "GraphicsCore/Components/VulkanDeviceComponent.hpp"
#include "GraphicsCore/Components/SkyboxComponent.hpp"
#include "GraphicsCore/Resources/Components/BindlessTextureDSetComponent.hpp"
#include "GraphicsCore/Resources/Components/GlobalDSetComponent.hpp"
#include "GraphicsCore/GraphicsContexts.hpp"
#include "GraphicsCore/Managers/PipelineManager.hpp"
#include "GraphicsCore/Components/PipelineManagerComponent.hpp"

void SkyboxFactory::loadSkybox(const std::string& hdrPath, GeneralManager& gm)
{
	TextureManager& textureManager =
	    *gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	DescriptorManager& descriptorManager =
	    *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
	PipelineManager& pipelineManager =
	    *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	BindlessTextureDSetComponent& bTextureDSetComponent =
	    *gm.getContextComponent<MainDSetsContext, BindlessTextureDSetComponent>();
	VmaAllocator allocator = gm.getContextComponent<VMAllocatorContext, VMAllocatorComponent>()->allocator;
	VulkanDevice& vulkanDevice =
	    *gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	SkyboxComponent& skybox = *gm.getContextComponent<SkyBoxContext, SkyboxComponent>();
	GlobalDSetComponent& globalDSetComp = *gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();

	// Upload HDR texture
	TextureHandle hdrHandle = textureManager.allocateTextureSlot();
	Texture& hdrTexture = textureManager.getTexture(hdrHandle);
	TextureUploader::uploadHdrTextureFromFile(hdrPath.c_str(), hdrTexture, textureManager, allocator, vulkanDevice);
	textureManager.registerTexturePath(hdrPath.c_str(), hdrHandle);

	descriptorManager.update(bTextureDSetComponent.bindlessTextureSet, Bindings::Textures::Array, 0,
	                         vk::DescriptorType::eCombinedImageSampler, hdrTexture.textureImageView,
	                         textureManager.getSampler(hdrTexture.samplerHandle),
	                         vk::ImageLayout::eShaderReadOnlyOptimal, static_cast<uint32_t>(hdrHandle.id));

	TextureHandle cubemapHandle = EnvMapFactory::cubemapFromHdr(
	    textureManager, vulkanDevice, hdrHandle, descriptorManager, bTextureDSetComponent, pipelineManager);

	// Bake skybox SH into probe slot 0 (the global fallback probe)
	EnvMapFactory::bakeSHForProbe(textureManager, vulkanDevice, cubemapHandle, 0, descriptorManager,
	                              bTextureDSetComponent, globalDSetComp.globalDSets, pipelineManager);

	TextureHandle prefilteredHandle = EnvMapFactory::prefilteredEnvMap(
	    textureManager, vulkanDevice, cubemapHandle, descriptorManager, bTextureDSetComponent, pipelineManager);
	descriptorManager.update(bTextureDSetComponent.bindlessTextureSet, Bindings::Textures::PrefilteredMap, 0,
	                         vk::DescriptorType::eCombinedImageSampler,
	                         textureManager.getTexture(prefilteredHandle).textureImageView,
	                         textureManager.getSampler(textureManager.getTexture(prefilteredHandle).samplerHandle));
	descriptorManager.update(bTextureDSetComponent.bindlessTextureSet, Bindings::Textures::BrdfLut, 0,
	                         vk::DescriptorType::eCombinedImageSampler,
	                         textureManager.getTexture(skybox.brdfLut).textureImageView,
	                         textureManager.getSampler(textureManager.getTexture(skybox.brdfLut).samplerHandle));

	skybox.cubemapTexture = cubemapHandle;
	skybox.prefilteredMap = prefilteredHandle;
	skybox.hasSkybox = true;
	// brdfLut stays the same - it's generated once at init
}
