#include "SkyboxFactory.hpp"

#include "../Managers/TextureManager.hpp"
#include "../Managers/DescriptorManager.hpp"
#include "TextureUploader.hpp"
#include "../../Components/TextureManagerComponent.hpp"
#include "../../Components/DescriptorManagerComponent.hpp"
#include "../../Components/PipelineHandlerComponent.hpp"
#include "../../Components/VMAllocatorComponent.hpp"
#include "../../Components/VulkanDeviceComponent.hpp"
#include "../../Components/SkyboxComponent.hpp"
#include "../Components/BindlessTextureDSetComponent.hpp"
#include "../../GraphicsContexts.hpp"
#include "../../PipelineHandler.hpp"

void SkyboxFactory::loadSkybox(const std::string& hdrPath, GeneralManager& gm)
{
	TextureManager* tManager =
	    gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	DescriptorManager* dManager =
	    gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
	PipelineHandler* pipelineHandler =
	    gm.getContextComponent<MainSignatureContext, PipelineHandlerComponent>()->pipelineHandler;
	BindlessTextureDSetComponent* bTextureDSetComponent =
	    gm.getContextComponent<MainDSetsContext, BindlessTextureDSetComponent>();
	VmaAllocator allocator =
	    gm.getContextComponent<VMAllocatorContext, VMAllocatorComponent>()->allocator;
	VulkanDevice* vulkanDevice =
	    gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	SkyboxComponent* skybox =
	    gm.getContextComponent<SkyBoxContext, SkyboxComponent>();

	// Upload HDR texture
	tManager->textures.push_back(Texture());
	Texture& hdrTexture = tManager->textures.back();
	TextureUploader::uploadHdrTextureFromFile(hdrPath.c_str(), hdrTexture, *tManager, allocator, *vulkanDevice);
	int hdrIndex = static_cast<int>(tManager->textures.size() - 1);
	tManager->texturePaths[hdrPath] = TextureHandle{hdrIndex};
	TextureHandle hdrHandle = tManager->texturePaths[hdrPath];

	dManager->updateBindlessTextureSet(hdrTexture.textureImageView, hdrTexture.textureSampler,
	                                   *bTextureDSetComponent, hdrIndex);

	// Generate cubemap, irradiance map, and prefiltered environment map from the HDR texture
	TextureHandle cubemapHandle =
	    tManager->generateCubemapFromHdr(hdrHandle, *pipelineHandler, *dManager, *bTextureDSetComponent);
	TextureHandle irradianceHandle =
	    tManager->generateIrradianceMap(cubemapHandle, *pipelineHandler, *dManager, *bTextureDSetComponent);
	TextureHandle prefilteredHandle =
	    tManager->generatePrefilteredEnvMap(cubemapHandle, *pipelineHandler, *dManager, *bTextureDSetComponent);

	// Restore cubemap descriptors to the actual environment cubemap (used by skybox)
	dManager->updateCubemapDescriptors(*bTextureDSetComponent,
	                                   tManager->textures[cubemapHandle.id].textureImageView,
	                                   tManager->textures[cubemapHandle.id].textureSampler,
	                                   tManager->textures[cubemapHandle.id].textureImageView);
	dManager->updateIBLDescriptors(*bTextureDSetComponent,
	                               tManager->textures[irradianceHandle.id].textureImageView,
	                               tManager->textures[irradianceHandle.id].textureSampler,
	                               tManager->textures[prefilteredHandle.id].textureImageView,
	                               tManager->textures[prefilteredHandle.id].textureSampler,
	                               tManager->textures[skybox->brdfLut.id].textureImageView,
	                               tManager->textures[skybox->brdfLut.id].textureSampler);

	skybox->cubemapTexture = cubemapHandle;
	skybox->irradianceMap = irradianceHandle;
	skybox->prefilteredMap = prefilteredHandle;
	// brdfLut stays the same - it's generated once at init
}
