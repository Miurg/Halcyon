#include "GraphicsInit.hpp"
#include <iostream>
#include <stdexcept>

#include "DeletionQueueComponent.hpp"
#include "DeletionQueueContext.hpp"
#include "../Factories/VulkanDeviceFactory.hpp"
#include "../Factories/SwapChainFactory.hpp"
#include "GraphicsCore/Components/VulkanDeviceComponent.hpp"
#include "GraphicsCore/Components/SwapChainComponent.hpp"
#include "GraphicsCore/Components/VMAllocatorComponent.hpp"
#include "GraphicsCore/Components/TextureManagerComponent.hpp"
#include "GraphicsCore/Components/BufferManagerComponent.hpp"
#include "GraphicsCore/Components/ModelManagerComponent.hpp"
#include "GraphicsCore/Components/DescriptorManagerComponent.hpp"
#include "GraphicsCore/Components/FrameManagerComponent.hpp"
#include "GraphicsCore/Components/FrameDataComponent.hpp"
#include "GraphicsCore/Components/CurrentFrameComponent.hpp"
#include "GraphicsCore/Components/FrameImageComponent.hpp"
#include "GraphicsCore/Components/TracyContextComponent.hpp"
#include "GraphicsCore/Resources/Components/GlobalDSetComponent.hpp"
#include "GraphicsCore/Resources/Components/ModelDSetComponent.hpp"
#include "GraphicsCore/Components/DrawInfoComponent.hpp"
#include "GraphicsCore/Components/NameComponent.hpp"
#include "PlatformCore/PlatformContexts.hpp"
#include "PlatformCore/Components/WindowComponent.hpp"
#include "GraphicsCore/VulkanDevice.hpp"
#include "GraphicsCore/SwapChain.hpp"
#include "GraphicsCore/Resources/Managers/TextureManager.hpp"
#include "GraphicsCore/Resources/Managers/BufferManager.hpp"
#include "GraphicsCore/Resources/Managers/ModelManager.hpp"
#include "GraphicsCore/Resources/Managers/DescriptorManager.hpp"
#include "../Managers/FrameManager.hpp"
#include "GraphicsCore/GraphicsContexts.hpp"
#include "GraphicsPipelinesInit.hpp"
#ifdef HALCYON_DEV_TOOLS
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#endif
#include "PlaceholdersInit.hpp"
#include "GraphicsCore/Systems/DeltaTimeSystem.hpp"
#include "GraphicsCore/Systems/TransformSystem.hpp"
#include "GraphicsCore/Systems/FrameBeginSystem.hpp"
#include "GraphicsCore/Systems/PhysSyncSystem.hpp"
#ifdef HALCYON_DEV_TOOLS
#include "GraphicsCore/Systems/ImGuiSystem.hpp"
#endif
#include "GraphicsCore/Systems/CameraMatrixSystem.hpp"
#include "GraphicsCore/Systems/LightUpdateSystem.hpp"
#include "GraphicsCore/Systems/LightProbeGIBakeSystem.hpp"
#include "GraphicsCore/Systems/ReflectionProbeUpdateSystem.hpp"
#include "GraphicsCore/Systems/BufferUpdateSystem.hpp"
#include "GraphicsCore/Systems/RenderSystem.hpp"
#include "GraphicsCore/Systems/FrameEndSystem.hpp"
#include "GraphicsCore/Components/DeltaTimeComponent.hpp"
#include "GraphicsCore/Systems/GPUParticlesSystem.hpp"

#ifdef TRACY_ENABLE
#include <tracy/TracyVulkan.hpp>
#endif

#pragma region Run
void GraphicsInit::Run(GeneralManager& gm)
{
#ifdef _DEBUG
	std::cout << "GRAPHICSINIT::RUN::Start init" << std::endl;
#endif //_DEBUG

	initVulkanCore(gm);
	initManagers(gm);
	initFrameData(gm);
	GraphicsPipelinesInit::initPipelines(gm);
	PlaceholdersInit::initPlaceholders(gm);
#ifdef HALCYON_DEV_TOOLS
	initImGui(gm);
#endif
	coreInit(gm);
	PlaceholdersInit::initAfterCorePlaceholders(gm);

#ifdef _DEBUG
	std::cout << "GRAPHICSINIT::RUN::Succes!" << std::endl;
#endif //_DEBUG
}
#pragma endregion

#pragma region coreInit
void GraphicsInit::coreInit(GeneralManager& gm)
{
	gm.registerSystem<DeltaTimeSystem>();
	gm.registerSystem<TransformSystem>()
	    .after<DeltaTimeSystem>()
	    .before<FrameBeginSystem>()
	    .reads<RelationshipComponent>()
	    .writes<LocalTransformComponent, GlobalTransformComponent>();

	gm.registerSystem<FrameBeginSystem>()
	    .after<DeltaTimeSystem>()
	    .before<FrameEndSystem>()
	    .writes<CurrentFrameComponent, FrameImageComponent>();
	gm.registerSystem<GPUParticlesSystem>()
	    .after<FrameBeginSystem>()
	    .before<FrameEndSystem>()
	    .reads<ParticleEmitorComponent, GlobalTransformComponent>();
	gm.registerSystem<PhysSyncSystem>()
	    .after<FrameBeginSystem>()
	    .before<BufferUpdateSystem>()
	    .reads<PhysTransformSnapshotComponent>()
	    .writes<GlobalTransformComponent>();
#ifdef HALCYON_DEV_TOOLS
	gm.registerSystem<ImGuiSystem>()
	    .after<FrameBeginSystem>()
	    .before<BufferUpdateSystem>()
	    .reads<NameComponent, RelationshipComponent, CameraComponent, GraphicsSettingsComponent, DeltaTimeComponent,
	           PhysBodyComponent>()
	    .writes<GlobalTransformComponent, LocalTransformComponent, DirectLightComponent, PointLightComponent,
	            GtaoSettingsComponent, LightProbeGridComponent>();
#endif
	gm.registerSystem<CameraMatrixSystem>()
	    .after<FrameBeginSystem>()
	    .before<BufferUpdateSystem>()
	    .reads<CameraComponent, GlobalTransformComponent, DirectLightComponent, CurrentFrameComponent>();
	gm.registerSystem<LightUpdateSystem>()
	    .after<FrameBeginSystem>()
	    .before<BufferUpdateSystem>()
	    .reads<GlobalTransformComponent, PointLightComponent, CurrentFrameComponent>();
	gm.registerSystem<LightProbeGIBakeSystem>().after<RenderSystem>().before<FrameEndSystem>();
	gm.registerSystem<ReflectionProbeUpdateSystem>()
	    .after<LightProbeGIBakeSystem>()
	    .before<FrameEndSystem>()
	    .reads<ReflectionProbeComponent, CurrentFrameComponent>();
	gm.registerSystem<BufferUpdateSystem>()
	    .after<FrameBeginSystem>()
	    .before<FrameEndSystem>()
	    .reads<GlobalTransformComponent, MeshInfoComponent, CurrentFrameComponent>()
	    .writes<DrawInfoComponent>();
	gm.registerSystem<RenderSystem>()
	    .after<BufferUpdateSystem>()
	    .before<FrameEndSystem>()
	    .reads<GlobalTransformComponent, MeshInfoComponent, DrawInfoComponent, CurrentFrameComponent>();
	gm.registerSystem<FrameEndSystem>().reads<FrameImageComponent>();
}
#pragma endregion

#pragma region initVulkanCore
void GraphicsInit::initVulkanCore(GeneralManager& gm)
{
	DeletionQueue* dq = gm.getContextComponent<DeletionQueueContext, DeletionQueueComponent>()->queue;

#ifdef _DEBUG
	std::cout << "GRAPHICSINIT::VULKANNEEDS::Start create vulkan needs" << std::endl;
#endif //_DEBUG
	// === Vulkan needs ===
	Window* window = gm.getContextComponent<MainWindowContext, WindowComponent>()->windowInstance;

	// Vulkan Device
	Orhescyon::Entity vulkanDeviceEntity = gm.createEntity();
	gm.registerContext<MainVulkanDeviceContext>(vulkanDeviceEntity);
	VulkanDevice* vulkanDevice = new VulkanDevice();
	VulkanDeviceFactory::createVulkanDevice(*window, *vulkanDevice);
	gm.addComponent<VulkanDeviceComponent>(vulkanDeviceEntity, vulkanDevice);
#ifdef TRACY_ENABLE
	gm.registerContext<TracyContextContext>(vulkanDeviceEntity);
	gm.addComponent<TracyContextComponent>(vulkanDeviceEntity, VulkanDeviceFactory::createTracyContext(*vulkanDevice));
#endif
	gm.addComponent<NameComponent>(vulkanDeviceEntity, "SYSTEM Vulkan Device");
	dq->push_function([vulkanDevice]() { delete vulkanDevice; });

#ifdef TRACY_ENABLE
	TracyContextComponent* tracyCtxComp = gm.getContextComponent<TracyContextContext, TracyContextComponent>();
	TracyVkCtx tracyCtx = tracyCtxComp->context;
	dq->push_function([tracyCtx]() { TracyVkDestroy(tracyCtx); });
#endif

	// VMA Allocator
	Orhescyon::Entity vmaAllocatorEntity = gm.createEntity();
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
	gm.addComponent<NameComponent>(vmaAllocatorEntity, "SYSTEM VMA Allocator");
	dq->push_function([allocator]() { vmaDestroyAllocator(allocator); });
}
#pragma endregion

#pragma region initManagers
void GraphicsInit::initManagers(GeneralManager& gm)
{
	DeletionQueue* dq = gm.getContextComponent<DeletionQueueContext, DeletionQueueComponent>()->queue;

	VulkanDevice* vulkanDevice =
	    gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	VmaAllocator allocator = gm.getContextComponent<VMAllocatorContext, VMAllocatorComponent>()->allocator;

	// Texture Manager
	Orhescyon::Entity textureManagerEntity = gm.createEntity();
	gm.registerContext<TextureManagerContext>(textureManagerEntity);
	TextureManager* textureManager = new TextureManager(*vulkanDevice, allocator);
	gm.addComponent<TextureManagerComponent>(textureManagerEntity, textureManager);
	gm.addComponent<NameComponent>(textureManagerEntity, "SYSTEM Texture Manager");
	dq->push_function([textureManager]() { delete textureManager; });

	// Buffer Manager
	Orhescyon::Entity bufferManagerEntity = gm.createEntity();
	gm.registerContext<BufferManagerContext>(bufferManagerEntity);
	BufferManager* bManager = new BufferManager(*vulkanDevice, allocator);
	gm.addComponent<BufferManagerComponent>(bufferManagerEntity, bManager);
	gm.addComponent<NameComponent>(bufferManagerEntity, "SYSTEM Buffer Manager");
	dq->push_function([bManager]() { delete bManager; });

	// Model Manager
	Orhescyon::Entity modelManagerEntity = gm.createEntity();
	gm.registerContext<ModelManagerContext>(modelManagerEntity);
	ModelManager* mManager = new ModelManager(*vulkanDevice, allocator);
	gm.addComponent<ModelManagerComponent>(modelManagerEntity, mManager);
	gm.addComponent<NameComponent>(modelManagerEntity, "SYSTEM Model Manager");
	dq->push_function([mManager]() { delete mManager; });

	// Descriptor Manager
	Orhescyon::Entity descriptorManagerEntity = gm.createEntity();
	gm.registerContext<DescriptorManagerContext>(descriptorManagerEntity);
	DescriptorManager* dManager = new DescriptorManager(*vulkanDevice);
	gm.addComponent<DescriptorManagerComponent>(descriptorManagerEntity, dManager);
	gm.addComponent<NameComponent>(descriptorManagerEntity, "SYSTEM Descriptor Manager");
	dq->push_function([dManager]() { delete dManager; });

	// Frame Manager
	Orhescyon::Entity frameManagerEntity = gm.createEntity();
	gm.registerContext<FrameManagerContext>(frameManagerEntity);
	FrameManager* fManager = new FrameManager(*vulkanDevice);
	gm.addComponent<FrameManagerComponent>(frameManagerEntity, fManager);
	gm.addComponent<NameComponent>(frameManagerEntity, "SYSTEM Frame Manager");
	dq->push_function([fManager]() { delete fManager; });
}
#pragma endregion

#pragma region initFrameData
void GraphicsInit::initFrameData(GeneralManager& gm)
{
	DeletionQueue* dq = gm.getContextComponent<DeletionQueueContext, DeletionQueueComponent>()->queue;

	FrameManager* fManager = gm.getContextComponent<FrameManagerContext, FrameManagerComponent>()->frameManager;
	VulkanDevice* vulkanDevice =
	    gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	Window* window = gm.getContextComponent<MainWindowContext, WindowComponent>()->windowInstance;
	TextureManager* tManager = gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;

	Orhescyon::Entity frameDataEntity = gm.createEntity();
	gm.registerContext<MainFrameDataContext>(frameDataEntity);
	gm.registerContext<CurrentFrameContext>(frameDataEntity);
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		fManager->initFrameData();
	}
	gm.addComponent<FrameDataComponent>(frameDataEntity, 0);
	gm.addComponent<CurrentFrameComponent>(frameDataEntity);
	gm.addComponent<DrawInfoComponent>(frameDataEntity);
	gm.addComponent<NameComponent>(frameDataEntity, "SYSTEM Frame Data");

	// Frame Images
	Orhescyon::Entity frameImageEntity = gm.createEntity();
	gm.registerContext<FrameImageContext>(frameImageEntity);
	gm.addComponent<FrameImageComponent>(frameImageEntity);
	gm.addComponent<NameComponent>(frameImageEntity, "SYSTEM Frame Image");

	// Swap Chain
	Orhescyon::Entity swapChainEntity = gm.createEntity();
	gm.registerContext<MainSwapChainContext>(swapChainEntity);
	SwapChain* swapChain = new SwapChain();
	SwapChainFactory::createSwapChain(*swapChain, *vulkanDevice, *window);
	gm.addComponent<SwapChainComponent>(swapChainEntity, swapChain);
	gm.addComponent<NameComponent>(swapChainEntity, "SYSTEM Swap Chain");
	dq->push_function(
	    [swapChain]()
	    {
		    SwapChainFactory::cleanupSwapChain(*swapChain);
		    delete swapChain;
	    });

	// Main Descriptor Sets
	Orhescyon::Entity mainDSetsEntity = gm.createEntity();
	gm.registerContext<MainDSetsContext>(mainDSetsEntity);
	gm.addComponent<BindlessTextureDSetComponent>(mainDSetsEntity);
	gm.addComponent<GlobalDSetComponent>(mainDSetsEntity);
	gm.addComponent<ModelDSetComponent>(mainDSetsEntity);
	gm.addComponent<NameComponent>(mainDSetsEntity, "SYSTEM Descriptor Sets");
}
#pragma endregion

#ifdef HALCYON_DEV_TOOLS
#pragma region initImGui
void GraphicsInit::initImGui(GeneralManager& gm)
{
	DeletionQueue* dq = gm.getContextComponent<DeletionQueueContext, DeletionQueueComponent>()->queue;

#ifdef _DEBUG
	std::cout << "GRAPHICSINIT::IMGUI::Start init ImGui" << std::endl;
#endif //_DEBUG

	VulkanDevice* vulkanDevice =
	    gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	Window* window = gm.getContextComponent<MainWindowContext, WindowComponent>()->windowInstance;
	SwapChain* swapChain = gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;

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
	// Backend creates and owns its own pool: font atlas + headroom for ImGui_ImplVulkan_AddTexture calls.
	initInfo.DescriptorPoolSize = IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE + 16;
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

	dq->push_function(
	    []()
	    {
		    ImGui_ImplVulkan_Shutdown();
		    ImGui_ImplGlfw_Shutdown();
		    ImGui::DestroyContext();
	    });

#ifdef _DEBUG
	std::cout << "GRAPHICSINIT::IMGUI::Succes!" << std::endl;
#endif //_DEBUG
}
#pragma endregion
#endif // HALCYON_DEV_TOOLS