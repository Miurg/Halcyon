
#pragma region Includes
#include "GraphicsInit.hpp"
#include <iostream>
#include <stdexcept>
#include "Factories/VulkanDeviceFactory.hpp"
#include "Factories/SwapChainFactory.hpp"
#include "Factories/PipelineFactory.hpp"
#include "Factories/CommandBufferFactory.hpp"

// Entities and Components
#include "Components/VulkanDeviceComponent.hpp"
#include "Components/SwapChainComponent.hpp"
#include "Components/VMAllocatorComponent.hpp"
#include "Components/TextureManagerComponent.hpp"
#include "Components/BufferManagerComponent.hpp"
#include "Components/ModelManagerComponent.hpp"
#include "Components/DescriptorManagerComponent.hpp"
#include "Components/FrameManagerComponent.hpp"
#include "Components/FrameDataComponent.hpp"
#include "Components/CurrentFrameComponent.hpp"
#include "Components/FrameImageComponent.hpp"
#include "Components/PipelineHandlerComponent.hpp"
#include "Resources/Components/GlobalDSetComponent.hpp"
#include "Resources/Components/ModelDSetComponent.hpp"
#include "Components/DrawInfoComponent.hpp"
#include "Components/SsaoSettingsComponent.hpp"

#include "Components/NameComponent.hpp"
#include "Components/CameraComponent.hpp"
#include "Components/LightComponent.hpp"
#include "Components/LocalTransformComponent.hpp"
#include "Components/GlobalTransformComponent.hpp"
#include "Components/RelationshipComponent.hpp"
#include "Components/SkyboxComponent.hpp"
#include "../Platform/PlatformContexts.hpp"
#include "../Platform/Components/WindowComponent.hpp"

// Managers and Resources
#include "VulkanDevice.hpp"
#include "SwapChain.hpp"
#include "PipelineHandler.hpp"
#include "Resources/Managers/TextureManager.hpp"
#include "Resources/Managers/BufferManager.hpp"
#include "Resources/Managers/ModelManager.hpp"
#include "Resources/Managers/DescriptorManager.hpp"
#include "Managers/FrameManager.hpp"
#include "Resources/Factories/TextureUploader.hpp"
// Contexts
#include "GraphicsContexts.hpp"
#include "Resources/ResourceStructures.hpp"

// ImGui
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#pragma endregion

#pragma region Run
void GraphicsInit::Run(GeneralManager& gm)
{
#ifdef _DEBUG
	std::cout << "GRAPHICSINIT::RUN::Start init" << std::endl;
#endif //_DEBUG

	initVulkanCore(gm);
	initManagers(gm);
	initFrameData(gm);
	initPipelines(gm);
	initScene(gm);
	initImGui(gm);

#ifdef _DEBUG
	std::cout << "GRAPHICSINIT::RUN::Succes!" << std::endl;
#endif //_DEBUG
}
#pragma endregion

#pragma region initVulkanCore
void GraphicsInit::initVulkanCore(GeneralManager& gm)
{
#ifdef _DEBUG
	std::cout << "GRAPHICSINIT::VULKANNEEDS::Start create vulkan needs" << std::endl;
#endif //_DEBUG
	// === Vulkan needs ===
	Window* window = gm.getContextComponent<MainWindowContext, WindowComponent>()->windowInstance;

	// Vulkan Device
	Entity vulkanDeviceEntity = gm.createEntity();
	gm.registerContext<MainVulkanDeviceContext>(vulkanDeviceEntity);
	VulkanDevice* vulkanDevice = new VulkanDevice();
	VulkanDeviceFactory::createVulkanDevice(*window, *vulkanDevice);
	gm.addComponent<VulkanDeviceComponent>(vulkanDeviceEntity, vulkanDevice);

	// VMA Allocator
	Entity vmaAllocatorEntity = gm.createEntity();
	gm.registerContext<VMAllocatorContext>(vmaAllocatorEntity);
	VmaAllocator allocator;
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = *vulkanDevice->physicalDevice;
	allocatorInfo.device = *vulkanDevice->device;
	allocatorInfo.instance = *vulkanDevice->instance;
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_4;
	VmaVulkanFunctions vulkanFunctions = {};
	vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

	allocatorInfo.pVulkanFunctions = &vulkanFunctions;
	VkResult result = vmaCreateAllocator(&allocatorInfo, &allocator);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create VMA allocator!");
	}
	gm.addComponent<VMAllocatorComponent>(vmaAllocatorEntity, allocator);
}
#pragma endregion

#pragma region initManagers
void GraphicsInit::initManagers(GeneralManager& gm)
{
	VulkanDevice* vulkanDevice =
	    gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	VmaAllocator allocator = gm.getContextComponent<VMAllocatorContext, VMAllocatorComponent>()->allocator;

	// Texture Manager
	Entity textureManagerEntity = gm.createEntity();
	gm.registerContext<TextureManagerContext>(textureManagerEntity);
	TextureManager* textureManager = new TextureManager(*vulkanDevice, allocator);
	gm.addComponent<TextureManagerComponent>(textureManagerEntity, textureManager);

	// Buffer Manager
	Entity bufferManagerEntity = gm.createEntity();
	gm.registerContext<BufferManagerContext>(bufferManagerEntity);
	BufferManager* bManager = new BufferManager(*vulkanDevice, allocator);
	gm.addComponent<BufferManagerComponent>(bufferManagerEntity, bManager);

	// Model Manager
	Entity modelManagerEntity = gm.createEntity();
	gm.registerContext<ModelManagerContext>(modelManagerEntity);
	ModelManager* mManager = new ModelManager(*vulkanDevice, allocator);
	gm.addComponent<ModelManagerComponent>(modelManagerEntity, mManager);

	// Descriptor Manager
	Entity descriptorManagerEntity = gm.createEntity();
	gm.registerContext<DescriptorManagerContext>(descriptorManagerEntity);
	DescriptorManager* dManager = new DescriptorManager(*vulkanDevice);
	gm.addComponent<DescriptorManagerComponent>(descriptorManagerEntity, dManager);

	// Frame Manager
	Entity frameManagerEntity = gm.createEntity();
	gm.registerContext<FrameManagerContext>(frameManagerEntity);
	FrameManager* fManager = new FrameManager(*vulkanDevice);
	gm.addComponent<FrameManagerComponent>(frameManagerEntity, fManager);
}
#pragma endregion

#pragma region initFrameData
void GraphicsInit::initFrameData(GeneralManager& gm)
{
	FrameManager* fManager = gm.getContextComponent<FrameManagerContext, FrameManagerComponent>()->frameManager;

	Entity frameDataEntity = gm.createEntity();
	gm.registerContext<MainFrameDataContext>(frameDataEntity);
	gm.registerContext<CurrentFrameContext>(frameDataEntity);
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		fManager->initFrameData();
	}
	gm.addComponent<FrameDataComponent>(frameDataEntity, 0);
	gm.addComponent<CurrentFrameComponent>(frameDataEntity);
	gm.addComponent<DrawInfoComponent>(frameDataEntity);

	// Frame Images
	Entity frameImageEntity = gm.createEntity();
	gm.registerContext<FrameImageContext>(frameImageEntity);
	gm.addComponent<FrameImageComponent>(frameImageEntity);
}
#pragma endregion

#pragma region initPipelines
void GraphicsInit::initPipelines(GeneralManager& gm)
{
	VulkanDevice* vulkanDevice =
	    gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	Window* window = gm.getContextComponent<MainWindowContext, WindowComponent>()->windowInstance;
	DescriptorManager* dManager =
	    gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
	TextureManager* tManager = gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;

	// Swap Chain
	Entity swapChainEntity = gm.createEntity();
	gm.registerContext<MainSwapChainContext>(swapChainEntity);
	SwapChain* swapChain = new SwapChain();
	SwapChainFactory::createSwapChain(*swapChain, *vulkanDevice, *window);
	swapChain->offscreenTextureHandle = tManager->createOffscreenImage(
	    swapChain->swapChainExtent.width, swapChain->swapChainExtent.height, swapChain->hdrFormat);
	swapChain->viewNormalsTextureHandle = tManager->createOffscreenImage(
	    swapChain->swapChainExtent.width, swapChain->swapChainExtent.height, vk::Format::eR16G16B16A16Sfloat);
	swapChain->ssaoTextureHandle = tManager->createOffscreenImage(
	    swapChain->swapChainExtent.width, swapChain->swapChainExtent.height, vk::Format::eR8Unorm);
	swapChain->ssaoBlurTextureHandle = tManager->createOffscreenImage(
	    swapChain->swapChainExtent.width, swapChain->swapChainExtent.height, vk::Format::eR8Unorm);
	swapChain->ssaoNoiseTextureHandle = tManager->createSsaoNoiseTexture();
	swapChain->depthTextureHandle =
	    tManager->createDepthImage(swapChain->swapChainExtent.width, swapChain->swapChainExtent.height);
	gm.addComponent<SwapChainComponent>(swapChainEntity, swapChain);

	// Main Descriptor Sets
	Entity mainDSetsEntity = gm.createEntity();
	gm.registerContext<MainDSetsContext>(mainDSetsEntity);
	gm.addComponent<BindlessTextureDSetComponent>(mainDSetsEntity);
	gm.addComponent<GlobalDSetComponent>(mainDSetsEntity);
	gm.addComponent<ModelDSetComponent>(mainDSetsEntity);

	// Signature and Pipelines
	Entity signatureEntity = gm.createEntity();
	gm.registerContext<MainSignatureContext>(signatureEntity);
	PipelineHandler* pipelineHandler = new PipelineHandler();
	PipelineFactory::createGraphicsPipeline(*vulkanDevice, *swapChain, *dManager, *pipelineHandler, *tManager);
	PipelineFactory::createAlphaTestPipeline(*vulkanDevice, *swapChain, *dManager, *pipelineHandler, *tManager);
	PipelineFactory::createDepthPrepassPipeline(*vulkanDevice, *swapChain, *dManager, *pipelineHandler, *tManager);
	PipelineFactory::createShadowPipeline(*vulkanDevice, *swapChain, *dManager, *pipelineHandler, *tManager);
	PipelineFactory::createFxaaPipeline(*vulkanDevice, *swapChain, *dManager, *pipelineHandler);
	PipelineFactory::createSsaoPipeline(*vulkanDevice, *swapChain, *dManager, *pipelineHandler);
	PipelineFactory::createSsaoBlurPipeline(*vulkanDevice, *swapChain, *dManager, *pipelineHandler);
	PipelineFactory::createCullingPipeline(*vulkanDevice, *dManager, *pipelineHandler);
	PipelineFactory::createEquirectToCubePipeline(*vulkanDevice, *dManager, *pipelineHandler);
	PipelineFactory::createSkyboxPipeline(*vulkanDevice, *swapChain, *pipelineHandler, *tManager);
	gm.addComponent<PipelineHandlerComponent>(signatureEntity, pipelineHandler);

#ifdef _DEBUG
	std::cout << "GRAPHICSINIT::VULKANNEEDS::Succes!" << std::endl;
#endif //_DEBUG
}
#pragma endregion

#pragma region initScene
void GraphicsInit::initScene(GeneralManager& gm)
{
#ifdef _DEBUG
	std::cout << "GRAPHICSINIT::PLACEHOLDERENTITYS::Start creating placeholder entities" << std::endl;
#endif //_DEBUG
#pragma region Fetch Contexts
	DescriptorManager* dManager =
	    gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
	BufferManager* bManager = gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	TextureManager* tManager = gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	BindlessTextureDSetComponent* bTextureDSetComponent =
	    gm.getContextComponent<MainDSetsContext, BindlessTextureDSetComponent>();
	GlobalDSetComponent* globalDSetComponent = gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	SwapChain* swap = gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	VmaAllocator allocator = gm.getContextComponent<VMAllocatorContext, VMAllocatorComponent>()->allocator;
	VulkanDevice* vulkanDevice =
	    gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	PipelineHandler* pipelineHandler =
	    gm.getContextComponent<MainSignatureContext, PipelineHandlerComponent>()->pipelineHandler;
#pragma endregion

#pragma region Scene Entities (Camera, Sun, Skybox)
	// === Camera ===
	Entity cameraEntity = gm.createEntity();
	gm.addComponent<NameComponent>(cameraEntity, "Main Camera");
	gm.addComponent<CameraComponent>(cameraEntity);
	gm.addComponent<GlobalTransformComponent>(cameraEntity, glm::vec3(-5.0f, 5.0f, 3.0f));
	gm.registerContext<MainCameraContext>(cameraEntity);
	CameraComponent* camera = gm.getContextComponent<MainCameraContext, CameraComponent>();
	

	// === Sun ===
	Entity sunEntity = gm.createEntity();
	gm.addComponent<NameComponent>(sunEntity, "Directional Light (Sun)");
	gm.addComponent<CameraComponent>(sunEntity);
	glm::vec3 sunPos = glm::vec3(10.0f, 20.0f, 10.0f);
	glm::vec3 sunDir = glm::normalize(-sunPos);
	glm::quat sunRot = glm::quatLookAt(sunDir, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::vec4 sunColor = glm::vec4(0.984f, 1.0f, 0.808f, 10.0f); // Slightly warm white light with high intensity
	glm::vec4 sunAmbient = sunColor * 0.1f;                      // Ambient component is 10% of the main light color
	gm.addComponent<GlobalTransformComponent>(sunEntity, sunPos, sunRot);
	gm.addComponent<LightComponent>(sunEntity, 8192, 8192, sunColor, sunAmbient);
	gm.registerContext<SunContext>(sunEntity);
	CameraComponent* sunCamera = gm.getContextComponent<SunContext, CameraComponent>();
	LightComponent* sunLight = gm.getContextComponent<SunContext, LightComponent>();
	sunLight->textureShadowImage = tManager->createShadowMap(sunLight->sizeX, sunLight->sizeY);

	// === Skybox ===
	Entity skyboxEntity = gm.createEntity();
	gm.addComponent<NameComponent>(skyboxEntity, "Skybox");
	gm.addComponent<SkyboxComponent>(skyboxEntity);
	gm.registerContext<SkyBoxContext>(skyboxEntity);
	SkyboxComponent* skybox = gm.getContextComponent<SkyBoxContext, SkyboxComponent>();
#pragma endregion

#pragma region Global Descriptor Set (Set 0)
	globalDSetComponent->globalDSets =
	    dManager->allocateStorageBufferDSets(MAX_FRAMES_IN_FLIGHT, *dManager->globalSetLayout);

	// Camera buffer
	globalDSetComponent->cameraBuffers =
	    bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal),
	                           sizeof(CameraStructure), MAX_FRAMES_IN_FLIGHT, vk::BufferUsageFlagBits::eStorageBuffer);
	dManager->updateStorageBufferDescriptors(*bManager, globalDSetComponent->cameraBuffers,
	                                         globalDSetComponent->globalDSets, 0);

	// Sun buffer
	globalDSetComponent->sunCameraBuffers =
	    bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal),
	                           sizeof(SunStructure), MAX_FRAMES_IN_FLIGHT, vk::BufferUsageFlagBits::eStorageBuffer);
	dManager->updateStorageBufferDescriptors(*bManager, globalDSetComponent->sunCameraBuffers,
	                                         globalDSetComponent->globalDSets, 1);
#pragma endregion

#pragma region Material & Texture System (Set 2)
	bTextureDSetComponent->bindlessTextureSet = dManager->allocateBindlessTextureDSet();

	// Material Buffer
	bTextureDSetComponent->materialBuffer =
	    bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible), 10240 * sizeof(MaterialStructure), 1,
	                           vk::BufferUsageFlagBits::eStorageBuffer);
	dManager->updateStorageBufferDescriptors(*bManager, bTextureDSetComponent->materialBuffer,
	                                         bTextureDSetComponent->bindlessTextureSet, 2);

	// Shadow Map Texture Set (binding 1)
	dManager->updateSingleTextureDSet(bTextureDSetComponent->bindlessTextureSet, 1,
	                                  tManager->textures[sunLight->textureShadowImage.id].textureImageView,
	                                  tManager->textures[sunLight->textureShadowImage.id].textureSampler);

	// Default White Texture
	auto texturePtr = GltfLoader::createDefaultWhiteTexture();
	int texWidth = texturePtr.get()->width;
	int texHeight = texturePtr.get()->height;
	auto data = texturePtr->pixels.data();
	auto path = texturePtr.get()->name.c_str();
	tManager->generateTextureData(path, texWidth, texHeight, data, *bTextureDSetComponent, *dManager);

	// HDR Skybox Texture
	std::string hdrPath = "assets/textures/sunflowers_puresky_4k.hdr";
	tManager->textures.push_back(Texture());
	Texture& hdrTexture = tManager->textures.back();
	TextureUploader::uploadHdrTextureFromFile(hdrPath.c_str(), hdrTexture, *tManager, allocator, *vulkanDevice);
	int hdrIndex = static_cast<int>(tManager->textures.size() - 1);
	tManager->texturePaths[hdrPath] = TextureHandle{hdrIndex};
	TextureHandle hdrHandle = tManager->texturePaths[hdrPath];

	dManager->updateBindlessTextureSet(hdrTexture.textureImageView, hdrTexture.textureSampler, *bTextureDSetComponent,
	                                   hdrIndex);
	TextureHandle cubemapHandle =
	    tManager->generateCubemapFromHdr(hdrHandle, *pipelineHandler, *dManager, *bTextureDSetComponent);

	skybox->cubemapTexture = cubemapHandle;
#pragma endregion

#pragma region Model & Frustum Culling Buffers (Set 1)
	ModelDSetComponent* objectDSetComponent = gm.getContextComponent<MainDSetsContext, ModelDSetComponent>();
	objectDSetComponent->modelBufferDSet =
	    dManager->allocateStorageBufferDSets(MAX_FRAMES_IN_FLIGHT, *dManager->modelSetLayout);

	objectDSetComponent->primitiveBuffer =
	    bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible), 10240 * sizeof(PrimitiveSctructure),
	                           MAX_FRAMES_IN_FLIGHT, vk::BufferUsageFlagBits::eStorageBuffer);
	dManager->updateStorageBufferDescriptors(*bManager, objectDSetComponent->primitiveBuffer,
	                                         objectDSetComponent->modelBufferDSet, 0);

	objectDSetComponent->transformBuffer =
	    bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible), 10240 * sizeof(TransformStructure),
	                           MAX_FRAMES_IN_FLIGHT, vk::BufferUsageFlagBits::eStorageBuffer);
	dManager->updateStorageBufferDescriptors(*bManager, objectDSetComponent->transformBuffer,
	                                         objectDSetComponent->modelBufferDSet, 1);

	objectDSetComponent->indirectDrawBuffer =
	    bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal),
	                           sizeof(IndirectDrawStructure) * 10240, MAX_FRAMES_IN_FLIGHT,
	                           vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer |
	                               vk::BufferUsageFlagBits::eTransferDst);
	dManager->updateStorageBufferDescriptors(*bManager, objectDSetComponent->indirectDrawBuffer,
	                                         objectDSetComponent->modelBufferDSet, 2);

	objectDSetComponent->visibleIndicesBuffer =
	    bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal),
	                           sizeof(uint32_t) * 10240, MAX_FRAMES_IN_FLIGHT, vk::BufferUsageFlagBits::eStorageBuffer);
	dManager->updateStorageBufferDescriptors(*bManager, objectDSetComponent->visibleIndicesBuffer,
	                                         objectDSetComponent->modelBufferDSet, 3);
#pragma endregion

#pragma region Post-Processing (FXAA)
	globalDSetComponent->fxaaDSets = dManager->allocateFxaaDescriptorSet(*dManager->screenSpaceSetLayout);
	dManager->updateSingleTextureDSet(globalDSetComponent->fxaaDSets, 0,
	                                  tManager->textures[swap->offscreenTextureHandle.id].textureImageView,
	                                  tManager->textures[swap->offscreenTextureHandle.id].textureSampler);
	dManager->updateSingleTextureDSet(globalDSetComponent->fxaaDSets, 1,
	                                  tManager->textures[swap->ssaoBlurTextureHandle.id].textureImageView,
	                                  tManager->textures[swap->ssaoBlurTextureHandle.id].textureSampler);
	dManager->updateSingleTextureDSet(globalDSetComponent->fxaaDSets, 2,
	                                  tManager->textures[swap->offscreenTextureHandle.id].textureImageView,
	                                  tManager->textures[swap->offscreenTextureHandle.id].textureSampler);
#pragma endregion

#pragma region SSAO Descriptor Sets
	globalDSetComponent->ssaoDSets = dManager->allocateFxaaDescriptorSet(*dManager->screenSpaceSetLayout);
	dManager->updateSingleTextureDSet(globalDSetComponent->ssaoDSets, 0,
	                                  tManager->textures[swap->depthTextureHandle.id].textureImageView,
	                                  tManager->textures[swap->depthTextureHandle.id].textureSampler);
	dManager->updateSingleTextureDSet(globalDSetComponent->ssaoDSets, 1,
	                                  tManager->textures[swap->viewNormalsTextureHandle.id].textureImageView,
	                                  tManager->textures[swap->viewNormalsTextureHandle.id].textureSampler);
	dManager->updateSingleTextureDSet(globalDSetComponent->ssaoDSets, 2,
	                                  tManager->textures[swap->ssaoNoiseTextureHandle.id].textureImageView,
	                                  tManager->textures[swap->ssaoNoiseTextureHandle.id].textureSampler);

	globalDSetComponent->ssaoBlurDSets = dManager->allocateFxaaDescriptorSet(*dManager->screenSpaceSetLayout);
	dManager->updateSingleTextureDSet(globalDSetComponent->ssaoBlurDSets, 0,
	                                  tManager->textures[swap->ssaoTextureHandle.id].textureImageView,
	                                  tManager->textures[swap->ssaoTextureHandle.id].textureSampler);
	dManager->updateSingleTextureDSet(globalDSetComponent->ssaoBlurDSets, 1,
	                                  tManager->textures[swap->offscreenTextureHandle.id].textureImageView,
	                                  tManager->textures[swap->offscreenTextureHandle.id].textureSampler);
	dManager->updateSingleTextureDSet(globalDSetComponent->ssaoBlurDSets, 2,
	                                  tManager->textures[swap->offscreenTextureHandle.id].textureImageView,
	                                  tManager->textures[swap->offscreenTextureHandle.id].textureSampler);
#pragma endregion

#pragma region SSAO Settings
	Entity ssaoSettingsEntity = gm.createEntity();
	gm.addComponent<NameComponent>(ssaoSettingsEntity, "SSAO Settings");
	gm.registerContext<SsaoSettingsContext>(ssaoSettingsEntity);
	gm.addComponent<SsaoSettingsComponent>(ssaoSettingsEntity);
#pragma endregion

#ifdef _DEBUG
	std::cout << "GRAPHICSINIT::PLACEHOLDERENTITYS::Succes!" << std::endl;
#endif //_DEBUG
}
#pragma endregion // initScene

#pragma region initImGui
void GraphicsInit::initImGui(GeneralManager& gm)
{
#ifdef _DEBUG
	std::cout << "GRAPHICSINIT::IMGUI::Start init ImGui" << std::endl;
#endif //_DEBUG

	VulkanDevice* vulkanDevice =
	    gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	Window* window = gm.getContextComponent<MainWindowContext, WindowComponent>()->windowInstance;
	SwapChain* swapChain = gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	DescriptorManager* dManager =
	    gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForVulkan(window->getHandle(), true);

	ImGui_ImplVulkan_InitInfo initInfo = {};
	initInfo.Instance = *vulkanDevice->instance;
	initInfo.PhysicalDevice = *vulkanDevice->physicalDevice;
	initInfo.Device = *vulkanDevice->device;
	initInfo.QueueFamily = vulkanDevice->graphicsIndex;
	initInfo.Queue = *vulkanDevice->graphicsQueue;
	initInfo.PipelineCache = nullptr;
	initInfo.DescriptorPool = *dManager->imguiPool;
	initInfo.RenderPass = nullptr; // For dynamic rendering
	initInfo.Subpass = 0;
	initInfo.MinImageCount = 2; // Usually 2 or 3
	initInfo.ImageCount = MAX_FRAMES_IN_FLIGHT;
	initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	initInfo.Allocator = nullptr;
	initInfo.CheckVkResultFn = nullptr;

	// Dynamic rendering specific setup
	VkPipelineRenderingCreateInfoKHR pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
	VkFormat colorAttachmentFormat = (VkFormat)swapChain->swapChainImageFormat;
	pipelineInfo.colorAttachmentCount = 1;
	pipelineInfo.pColorAttachmentFormats = &colorAttachmentFormat;

	initInfo.UseDynamicRendering = true;
	initInfo.PipelineRenderingCreateInfo = pipelineInfo;

	ImGui_ImplVulkan_Init(&initInfo);

	ImGui_ImplVulkan_CreateFontsTexture();

#ifdef _DEBUG
	std::cout << "GRAPHICSINIT::IMGUI::Succes!" << std::endl;
#endif //_DEBUG
}
#pragma endregion
