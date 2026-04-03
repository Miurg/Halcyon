#include "SkyboxFactory.hpp"

#include "../Managers/TextureManager.hpp"
#include "../Managers/DescriptorManager.hpp"
#include "../Managers/BufferManager.hpp"
#include "TextureUploader.hpp"
#include "../../Components/TextureManagerComponent.hpp"
#include "../../Components/DescriptorManagerComponent.hpp"
#include "../../Components/VMAllocatorComponent.hpp"
#include "../../Components/VulkanDeviceComponent.hpp"
#include "../../Components/SkyboxComponent.hpp"
#include "../../Components/BufferManagerComponent.hpp"
#include "../Components/BindlessTextureDSetComponent.hpp"
#include "../../GraphicsContexts.hpp"
#include "../../Managers/PipelineManager.hpp"
#include "../../Components/PipelineManagerComponent.hpp"

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

	// Upload HDR texture
	tManager.textures.push_back(Texture());
	Texture& hdrTexture = tManager.textures.back();
	TextureUploader::uploadHdrTextureFromFile(hdrPath.c_str(), hdrTexture, tManager, allocator, vulkanDevice);
	int hdrIndex = static_cast<int>(tManager.textures.size() - 1);
	tManager.texturePaths[hdrPath] = TextureHandle{hdrIndex};
	TextureHandle hdrHandle = tManager.texturePaths[hdrPath];

	dManager.updateBindlessTextureSet(hdrTexture.textureImageView, hdrTexture.textureSampler,
	                                   bTextureDSetComponent, hdrIndex);

	TextureHandle cubemapHandle =
	    tManager.generateCubemapFromHdr(hdrHandle, dManager, bTextureDSetComponent, pManager);

	BufferHandle shHandle =
	    bManager.generateSHCoefficients(cubemapHandle, dManager, bTextureDSetComponent, pManager, tManager);

	TextureHandle prefilteredHandle = tManager.generatePrefilteredEnvMap(cubemapHandle, dManager,
	                                                                      bTextureDSetComponent, pManager);
	dManager.updateIBLDescriptors(bTextureDSetComponent,
	                               tManager.textures[prefilteredHandle.id].textureImageView,
	                               tManager.textures[prefilteredHandle.id].textureSampler,
	                               tManager.textures[skybox.brdfLut.id].textureImageView,
	                               tManager.textures[skybox.brdfLut.id].textureSampler);

	skybox.cubemapTexture = cubemapHandle;
	skybox.shBuffer = shHandle;
	skybox.prefilteredMap = prefilteredHandle;
	skybox.hasSkybox = true;
	// brdfLut stays the same - it's generated once at init
}
