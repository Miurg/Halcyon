#include "GraphicsCore/Resources/Factories/SkyboxFactory.hpp"

#include "GraphicsCore/Resources/Managers/TextureManager.hpp"
#include "GraphicsCore/Resources/Managers/DescriptorManager.hpp"
#include "GraphicsCore/Resources/Managers/Bindings.hpp"
#include "GraphicsCore/Resources/Managers/BufferManager.hpp"
#include "GraphicsCore/Resources/Factories/TextureUploader.hpp"
#include "GraphicsCore/Components/TextureManagerComponent.hpp"
#include "GraphicsCore/Components/DescriptorManagerComponent.hpp"
#include "GraphicsCore/Components/VMAllocatorComponent.hpp"
#include "GraphicsCore/Components/VulkanDeviceComponent.hpp"
#include "GraphicsCore/Components/SkyboxComponent.hpp"
#include "GraphicsCore/Components/BufferManagerComponent.hpp"
#include "GraphicsCore/Resources/Components/BindlessTextureDSetComponent.hpp"
#include "GraphicsCore/Resources/Components/GlobalDSetComponent.hpp"
#include "GraphicsCore/GraphicsContexts.hpp"
#include "GraphicsCore/Managers/PipelineManager.hpp"
#include "GraphicsCore/Components/PipelineManagerComponent.hpp"

void SkyboxFactory::loadSkybox(const std::string& hdrPath, GeneralManager& gm)
{
	TextureManager& tManager =
	    *gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	DescriptorManager& dManager =
	    *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
	PipelineManager& pManager =
	    *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	BindlessTextureDSetComponent& bTextureDSetComponent =
	    *gm.getContextComponent<MainDSetsContext, BindlessTextureDSetComponent>();
	VmaAllocator allocator =
	    gm.getContextComponent<VMAllocatorContext, VMAllocatorComponent>()->allocator;
	VulkanDevice& vulkanDevice =
	    *gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	BufferManager& bManager =
	    *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	SkyboxComponent& skybox =
	    *gm.getContextComponent<SkyBoxContext, SkyboxComponent>();
	GlobalDSetComponent& globalDSetComp =
	    *gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();

	// Upload HDR texture
	int hdrIndex = tManager.allocateTextureSlot();
	Texture& hdrTexture = tManager.getTexture(TextureHandle{hdrIndex});
	TextureUploader::uploadHdrTextureFromFile(hdrPath.c_str(), hdrTexture, tManager, allocator, vulkanDevice);
	tManager.texturePaths[hdrPath] = TextureHandle{hdrIndex};
	TextureHandle hdrHandle = tManager.texturePaths[hdrPath];

	dManager.update(bTextureDSetComponent.bindlessTextureSet, Bindings::Textures::Array, 0,
	                vk::DescriptorType::eCombinedImageSampler, hdrTexture.textureImageView, hdrTexture.textureSampler,
	                vk::ImageLayout::eShaderReadOnlyOptimal, static_cast<uint32_t>(hdrIndex));

	TextureHandle cubemapHandle =
	    tManager.generateCubemapFromHdr(hdrHandle, dManager, bTextureDSetComponent, pManager);

	// Bake skybox SH into probe slot 0 (the global fallback probe)
	bManager.bakeSHForProbe(cubemapHandle, globalDSetComp.shProbeBuffer, 0,
	                         dManager, bTextureDSetComponent,
	                         globalDSetComp.globalDSets, pManager, tManager);

	TextureHandle prefilteredHandle = tManager.generatePrefilteredEnvMap(cubemapHandle, dManager,
	                                                                      bTextureDSetComponent, pManager);
	dManager.update(bTextureDSetComponent.bindlessTextureSet, Bindings::Textures::PrefilteredMap, 0,
	                vk::DescriptorType::eCombinedImageSampler, tManager.getTexture(prefilteredHandle).textureImageView,
	                tManager.getTexture(prefilteredHandle).textureSampler);
	dManager.update(bTextureDSetComponent.bindlessTextureSet, Bindings::Textures::BrdfLut, 0,
	                vk::DescriptorType::eCombinedImageSampler, tManager.getTexture(skybox.brdfLut).textureImageView,
	                tManager.getTexture(skybox.brdfLut).textureSampler);

	skybox.cubemapTexture = cubemapHandle;
	skybox.prefilteredMap = prefilteredHandle;
	skybox.hasSkybox = true;
	// brdfLut stays the same - it's generated once at init
}
