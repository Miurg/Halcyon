#include "BufferUpdateSystem.hpp"
#include "../Components/CameraComponent.hpp"
#include "../Components/TransformComponent.hpp"
#include "../Resources/Components/ModelsBuffersComponent.hpp"
#include "../Resources/Managers/BufferManager.hpp"
#include <iostream>
#include <chrono>
#include <vulkan/vulkan_raii.hpp>
#include "../../Core/GeneralManager.hpp"
#include "../GraphicsContexts.hpp"
#include "../Components/SwapChainComponent.hpp"
#include "../Components/BufferManagerComponent.hpp"
#include "../FrameData.hpp"
#include "../Components/FrameDataComponent.hpp"
#include "../Components/CurrentFrameComponent.hpp"

void BufferUpdateSystem::processEntity(Entity entity, GeneralManager& manager, float dt) {}

void BufferUpdateSystem::onRegistered(GeneralManager& gm) 
{
	std::cout << "BufferUpdateSystem registered!" << std::endl;
}

void BufferUpdateSystem::onShutdown(GeneralManager& gm) 
{
	std::cout << "BufferUpdateSystem shutdown!" << std::endl;
}

void BufferUpdateSystem::update(float deltaTime, GeneralManager& gm, const std::vector<Entity>& entities) 
{
	CurrentFrameComponent* currentFrameComp = gm.getContextComponent<CurrentFrameContext, CurrentFrameComponent>();
	uint32_t currentFrame = currentFrameComp->currentFrame;
	SwapChain& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	BufferManager& bufferManager =
	    *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	ModelsBuffersComponent* ssbos = gm.getContextComponent<ModelSSBOsContext, ModelsBuffersComponent>();
	CameraComponent* mainCamera = gm.getContextComponent<MainCameraContext, CameraComponent>();
	TransformComponent* mainCameraTransform = gm.getContextComponent<MainCameraContext, TransformComponent>();
	CameraComponent* sunCamera = gm.getContextComponent<LightCameraContext, CameraComponent>();
	TransformComponent* sunCameraTransform = gm.getContextComponent<LightCameraContext, TransformComponent>();
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	std::vector<TransformComponent*> transforms;
	transforms.reserve(entities.size());
	for (const auto& entity : entities)
	{
		transforms.push_back(gm.getComponent<TransformComponent>(entity));
	}

	// === Sun ===
	glm::vec3 lightPos = glm::vec3(20.0f + mainCameraTransform->position.x, 20.0f + mainCameraTransform->position.y,
	                               20.0f + mainCameraTransform->position.z);
	glm::vec3 lightTarget =
	    glm::vec3(mainCameraTransform->position.x, mainCameraTransform->position.y, mainCameraTransform->position.z);
	glm::mat4 lightView = glm::lookAt(lightPos, lightTarget, glm::vec3(0.0f, 1.0f, 0.0f));

	float orthoSize = 15.0f;
	glm::mat4 lightProj = glm::orthoRH_ZO(-orthoSize, orthoSize, -orthoSize, orthoSize, 0.1f, 500.0f);
	lightProj[1][1] *= -1; // Y-flip

	glm::mat4 lightSpaceMatrix = lightProj * lightView;

	CameraStucture sunUbo;
	sunUbo.view = lightView;
	sunUbo.proj = lightProj;
	sunUbo.lightSpaceMatrix = lightSpaceMatrix;

	memcpy(bufferManager.buffers[sunCamera->descriptorNumber].bufferMapped[currentFrame], &sunUbo, sizeof(sunUbo));

	// === Camera ===
	glm::mat4 view = mainCameraTransform->getViewMatrix();
	glm::mat4 proj = glm::perspective(glm::radians(mainCamera->fov),
	                                  static_cast<float>(swapChain.swapChainExtent.width) /
	                                      static_cast<float>(swapChain.swapChainExtent.height),
	                                  0.1f, 2000.0f);
	proj[1][1] *= -1; // Y-flip

	CameraStucture cameraUbo{.view = view, .proj = proj, .lightSpaceMatrix = lightSpaceMatrix};
	memcpy(bufferManager.buffers[mainCamera->descriptorNumber].bufferMapped[currentFrame], &cameraUbo,
	       sizeof(cameraUbo));

	// === Models ===
	glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 initialRotation =
	    glm::rotate(glm::mat4(1.0f), time / 10 * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 finalRotation = rotation * initialRotation;

	std::vector<ModelSctructure> frameData;
	frameData.reserve(entities.size());

	for (size_t i = 0; i < entities.size(); i++)
	{
		glm::mat4 model = transforms[i]->getModelMatrix() * finalRotation;
		ModelSctructure modelUbo{.model = glm::transpose(model)};
		frameData.push_back(modelUbo);
	}

	void* mappedMemory = bufferManager.buffers[ssbos->descriptorNumber].bufferMapped[currentFrame];
	memcpy(mappedMemory, frameData.data(), frameData.size() * sizeof(ModelSctructure));
}