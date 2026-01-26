#include "Core/GeneralManager.hpp"
#include "Graphics/FrameData.hpp"
#include "Graphics/PipelineHandler.hpp"
#include "Platform/Window.hpp"
#include "Graphics/SwapChain.hpp"
#include "Graphics/VulkanDevice.hpp"
#include "Core/GeneralManager.hpp"
#include "Graphics/Resources/Managers/BufferManager.hpp"
#include "Factories/VulkanDeviceFactory.hpp"
#include "Components/VulkanDeviceComponent.hpp"
#include "Factories/SwapChainFactory.hpp"
#include "Components/SwapChainComponent.hpp"
#include "Components/BufferManagerComponent.hpp"
#include "Components/DescriptorManagerComponent.hpp"
#include "Resources/Components/GlobalDSetComponent.hpp"
#include "Resources/Components/ModelDSetComponent.hpp"
#include "../Core/Entitys/EntityManager.hpp"
#include "Factories/PipelineFactory.hpp"
#include "Components/PipelineHandlerComponent.hpp"
#include "Components/FrameImageComponent.hpp"
#include "Components/FrameDataComponent.hpp"
#include "Components/CurrentFrameComponent.hpp"
#include "../Game/Components/ControlComponent.hpp"
#include "Components/LightComponent.hpp"
#include "GraphicsContexts.hpp"
#include "Components/CameraComponent.hpp"
#include "Components/LocalTransformComponent.hpp"
#include "Components/RelationshipComponent.hpp"
class GraphicsInit
{
public:
	static void Run(GeneralManager& gm)
	{
#ifdef _DEBUG
		std::cout << "GRAPHICSINIT::RUN::Start init" << std::endl;
#endif //_DEBUG
		vulkanNeedsCreate(gm);
		placeholderEntitysCreate(gm);
#ifdef _DEBUG
		std::cout << "GRAPHICSINIT::RUN::Succes!" << std::endl;
#endif //_DEBUG
	};

private:
	static void vulkanNeedsCreate(GeneralManager& gm) 
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

		// Swap Chain
		Entity swapChainEntity = gm.createEntity();
		gm.registerContext<MainSwapChainContext>(swapChainEntity);
		SwapChain* swapChain = new SwapChain();
		SwapChainFactory::createSwapChain(*swapChain, *vulkanDevice, *window);
		gm.addComponent<SwapChainComponent>(swapChainEntity, swapChain);

		// Buffer Manager
		Entity bufferManagerEntity = gm.createEntity();
		gm.registerContext<BufferManagerContext>(bufferManagerEntity);
		BufferManager* bManager = new BufferManager(*vulkanDevice);
		gm.addComponent<BufferManagerComponent>(bufferManagerEntity, bManager);

		// Descriptor Manager
		Entity descriptorManagerEntity = gm.createEntity();
		gm.registerContext<DescriptorManagerContext>(descriptorManagerEntity);
		DescriptorManager* dManager = new DescriptorManager(*vulkanDevice);
		gm.addComponent<DescriptorManagerComponent>(descriptorManagerEntity, dManager);

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
		PipelineFactory::createGraphicsPipeline(*vulkanDevice, *swapChain, *dManager, *pipelineHandler);
		PipelineFactory::createShadowPipeline(*vulkanDevice, *swapChain, *dManager, *pipelineHandler);
		gm.addComponent<PipelineHandlerComponent>(signatureEntity, pipelineHandler);

		// Frame Data and Images
		Entity frameImageEntity = gm.createEntity();
		gm.registerContext<FrameImageContext>(frameImageEntity);
		gm.addComponent<FrameImageComponent>(frameImageEntity);

		Entity frameDataEntity = gm.createEntity();
		gm.registerContext<MainFrameDataContext>(frameDataEntity);
		gm.registerContext<CurrentFrameContext>(frameDataEntity);
		std::vector<FrameData>* framesData = new std::vector<FrameData>(MAX_FRAMES_IN_FLIGHT);
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			FrameData::initFrameData((*framesData)[i], *vulkanDevice);
		}
		gm.addComponent<FrameDataComponent>(frameDataEntity, framesData);
		gm.addComponent<CurrentFrameComponent>(frameDataEntity);

		// === Vulkan needs END ===
#ifdef _DEBUG
		std::cout << "GRAPHICSINIT::VULKANNEEDS::Succes!" << std::endl;
#endif //_DEBUG
	}
	static void placeholderEntitysCreate(GeneralManager& gm)
	{
#ifdef _DEBUG
		std::cout << "GRAPHICSINIT::PLACEHOLDERENTITYS::Start creating placeholder entities" << std::endl;
#endif //_DEBUG
		// === Placeholder Entities ===

		// === Cameras and Lights ===
		Entity cameraEntity = gm.createEntity();
		gm.addComponent<CameraComponent>(cameraEntity);
		gm.addComponent<GlobalTransformComponent>(cameraEntity, glm::vec3(0.0f, 0.0f, 3.0f));
		gm.addComponent<ControlComponent>(cameraEntity);
		gm.registerContext<MainCameraContext>(cameraEntity);

		Entity sunEntity = gm.createEntity();
		gm.addComponent<CameraComponent>(sunEntity);
		gm.addComponent<GlobalTransformComponent>(sunEntity, glm::vec3(10.0f, 20.0f, 10.0f));
		gm.addComponent<LightComponent>(sunEntity, 2048, 2048, glm::vec4(1.0f, 1.0f, 1.0f, 3.0f),
		                                glm::vec4(1.0f, 1.0f, 1.0f, 0.1f));
		gm.registerContext<SunContext>(sunEntity);

		CameraComponent* camera = gm.getContextComponent<MainCameraContext, CameraComponent>();
		CameraComponent* sunCamera = gm.getContextComponent<SunContext, CameraComponent>();
		LightComponent* sunLight = gm.getContextComponent<SunContext, LightComponent>();

		DescriptorManager* dManager =
		    gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
		BufferManager* bManager = gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;

		BindlessTextureDSetComponent* bTextureDSetComponent =
		    gm.getContextComponent<MainDSetsContext, BindlessTextureDSetComponent>();
		bTextureDSetComponent->bindlessTextureSet = dManager->allocateBindlessTextureDSet();

		GlobalDSetComponent* globalDSetComponent = gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
		globalDSetComponent->globalDSets =
		    dManager->allocateStorageBufferDSets(MAX_FRAMES_IN_FLIGHT, *dManager->globalSetLayout);
		globalDSetComponent->cameraBuffers =
		    bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal),
		                           sizeof(CameraStucture), MAX_FRAMES_IN_FLIGHT, 0, *dManager->globalSetLayout);
		globalDSetComponent->sunCameraBuffers =
		    bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal),
		                           sizeof(SunStructure), MAX_FRAMES_IN_FLIGHT, 2, *dManager->globalSetLayout);

		dManager->updateStorageBufferDescriptors(*bManager, globalDSetComponent->cameraBuffers,
		                                         globalDSetComponent->globalDSets, 0);
		dManager->updateStorageBufferDescriptors(*bManager, globalDSetComponent->sunCameraBuffers,
		                                         globalDSetComponent->globalDSets, 2);

		sunLight->textureShadowImage = bManager->createShadowMap(sunLight->sizeX, sunLight->sizeY);
		dManager->updateShadowDSet(globalDSetComponent->globalDSets,
		                           bManager->textures[sunLight->textureShadowImage].textureImageView,
		                           bManager->textures[sunLight->textureShadowImage].textureSampler);

		// === Cameras and Lights END ===

		ModelDSetComponent* objectDSetComponent = gm.getContextComponent<MainDSetsContext, ModelDSetComponent>();
		objectDSetComponent->modelBufferDSet =
		    dManager->allocateStorageBufferDSets(MAX_FRAMES_IN_FLIGHT, *dManager->modelSetLayout);
		// === Model SSBOs ===
		
		objectDSetComponent->primitiveBuffer =
		    bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible), 10240 * sizeof(PrimitiveSctructure),
		                           MAX_FRAMES_IN_FLIGHT, 0, *dManager->modelSetLayout);
		dManager->updateStorageBufferDescriptors(*bManager, objectDSetComponent->primitiveBuffer,
		                                         objectDSetComponent->modelBufferDSet, 0);
		// === Model SSBOs END ===

		// === Primitives ===

		objectDSetComponent->transformBuffer =
		    bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible), 10240 * sizeof(TransformStructure),
		                           MAX_FRAMES_IN_FLIGHT, 1, *dManager->modelSetLayout);
		dManager->updateStorageBufferDescriptors(*bManager, objectDSetComponent->primitiveBuffer,
		                                         objectDSetComponent->modelBufferDSet, 1);

		// === Primitives END ===

		// === Placeholder Entities END ===
#ifdef _DEBUG
		std::cout << "GRAPHICSINIT::PLACEHOLDERENTITYS::Succes!" << std::endl;
#endif //_DEBUG
	}
};