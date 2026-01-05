#include "App.hpp"

#include <iostream>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLFW/glfw3.h>
#include "CoreInit.hpp"
#include "Graphics/Factories/VulkanDeviceFactory.hpp"
#include "Graphics/Factories/SwapChainFactory.hpp"
#include "Graphics/Factories/PipelineFactory.hpp"

#include "Platform/Components/WindowComponent.hpp"
#include "Platform/Components/KeyboardStateComponent.hpp"
#include "Platform/Components/MouseStateComponent.hpp"
#include "Platform/Components/CursorPositionComponent.hpp"
#include "Platform/Components/WindowSizeComponent.hpp"
#include "Platform/Components/ScrollDeltaComponent.hpp"
#include "Platform/Systems/InputSolverSystem.hpp"
#include "Platform/PlatformContexts.hpp"
#include "Graphics/Components/VulkanDeviceComponent.hpp"
#include "Graphics/GraphicsContexts.hpp"
#include "Graphics/Components/SwapChainComponent.hpp"
#include "Graphics/Components/BufferManagerComponent.hpp"
#include "Graphics/Components/PipelineHandlerComponent.hpp"
#include "Graphics/Components/FrameDataComponent.hpp"
#include "Graphics/Components/CurrentFrameComponent.hpp"
#include "Graphics/Systems/RenderSystem.hpp"
#include "Graphics/Components/TransformComponent.hpp"

namespace
{
float deltaTime = 0.0f;
float lastFrame = 0.0f;
} // anonymous namespace

App::App() {}

void App::run()
{
	//=== ECS ===
	GeneralManager gm;

	CoreInit::Run(gm);

	//=== ECS END ===

	window = gm.getContextComponent<MainWindowContext, WindowComponent>()->windowInstance;

	Entity vulkanDeviceEntity = gm.createEntity();
	gm.registerContext<MainVulkanDeviceContext>(vulkanDeviceEntity);
	vulkanDevice = new VulkanDevice();
	VulkanDeviceFactory::createVulkanDevice(*window, *vulkanDevice);
	gm.addComponent<VulkanDeviceComponent>(vulkanDeviceEntity, vulkanDevice);

	Entity cameraEntity = gm.createEntity();
	gm.addComponent<CameraComponent>(cameraEntity);
	gm.addComponent<TransformComponent>(cameraEntity, glm::vec3(0.0f, 0.0f, 3.0f));
	gm.registerContext<MainCameraContext>(cameraEntity);

	Entity sunEntity = gm.createEntity();
	gm.addComponent<CameraComponent>(sunEntity);
	gm.addComponent<TransformComponent>(sunEntity, glm::vec3(10.0f, 20.0f, 10.0f));
	gm.registerContext<LightCameraContext>(sunEntity);
	CameraComponent* camera = gm.getContextComponent<MainCameraContext, CameraComponent>();
	CameraComponent* sun = gm.getContextComponent<LightCameraContext, CameraComponent>();

	Entity swapChainEntity = gm.createEntity();
	gm.registerContext<MainSwapChainContext>(swapChainEntity);
	swapChain = new SwapChain();
	SwapChainFactory::createSwapChain(*swapChain, *vulkanDevice, *window);
	gm.addComponent<SwapChainComponent>(swapChainEntity, swapChain);

	Entity bufferManagerEntity = gm.createEntity();
	gm.registerContext<BufferManagerContext>(bufferManagerEntity);
	bufferManager = new BufferManager(*vulkanDevice);
	gm.addComponent<BufferManagerComponent>(bufferManagerEntity, bufferManager);

	Entity signatureEntity = gm.createEntity();
	gm.registerContext<MainSignatureContext>(signatureEntity);

	camera->descriptorNumber = bufferManager->createBuffer(
	    (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal), 1, sizeof(CameraStucture),
	    MAX_FRAMES_IN_FLIGHT, 0, *bufferManager->globalSetLayout);
	bufferManager->bindShadowMap(camera->descriptorNumber, swapChain->shadowImageView, *swapChain->shadowSampler);

	sun->descriptorNumber = bufferManager->createBuffer(
	    (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal), 1, sizeof(CameraStucture),
	    MAX_FRAMES_IN_FLIGHT, 0, *bufferManager->globalSetLayout);
	// We dont use it but must for vulkan
	bufferManager->bindShadowMap(sun->descriptorNumber, swapChain->shadowImageView, *swapChain->shadowSampler);

	pipelineHandler = new PipelineHandler();
	PipelineFactory::createGraphicsPipeline(*vulkanDevice, *swapChain, *bufferManager, *pipelineHandler);
	PipelineFactory::createShadowPipeline(*vulkanDevice, *swapChain, *bufferManager, *pipelineHandler);
	gm.addComponent<PipelineHandlerComponent>(signatureEntity, pipelineHandler);

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

	Entity modelSSBOsEntity = gm.createEntity();
	gm.registerContext<ModelSSBOsContext>(modelSSBOsEntity);
	int descriptorNumber = bufferManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible), 1000, sizeof(ModelSctructure),
	                                MAX_FRAMES_IN_FLIGHT, 0, *bufferManager->modelSetLayout);
	gm.addComponent<ModelsBuffersComponent>(modelSSBOsEntity, descriptorNumber);

	App::setupGameObjects(gm);
	App::mainLoop(gm);
	App::cleanup();
}

void App::mainLoop(GeneralManager& gm)
{
	while (!window->shouldClose())
	{
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		glfwPollEvents();
		gm.update(deltaTime);

		frameCount++;
		float now = static_cast<float>(glfwGetTime());
		if (now - time >= 1.0f)
		{
			std::cout << "FPS: " << frameCount << std::endl;
			frameCount = 0;
			time = now;
		}
	}
}

void App::cleanup()
{
	vulkanDevice->device.waitIdle();

	SwapChainFactory::cleanupSwapChain(*swapChain);
	bufferManager->~BufferManager();
}

void App::setupGameObjects(GeneralManager& gm)
{
	int j = 0;
	int k = 0;
	for (int i = 0; i < 100; i++)
	{
		MeshInfoComponent meshInfo;
		int numberTexture;
		if (i > 50)
		{
			meshInfo = bufferManager->createMesh("assets/models/BlenderMonkey.obj");
			numberTexture = bufferManager->generateTextureData("assets/textures/texture.jpg");
		}
		else
		{
			meshInfo = bufferManager->createMesh("assets/models/viking_room.obj");
			numberTexture = bufferManager->generateTextureData("assets/textures/viking_room.png");
		}

		Entity gameObjectEntity1 = gm.createEntity();
		gm.addComponent<TransformComponent>(gameObjectEntity1, k * 2.0f, 0.0f, j * 2.0f);
		gm.addComponent<MeshInfoComponent>(gameObjectEntity1, meshInfo);
		gm.addComponent<TextureInfoComponent>(gameObjectEntity1, numberTexture);
		gm.subscribeEntity<RenderSystem>(gameObjectEntity1);

		k++;
		if ((i + 1) % 10 == 0)
		{
			j++;
			k = 0;
		}
	}
}