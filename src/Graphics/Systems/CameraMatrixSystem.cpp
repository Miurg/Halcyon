#include "CameraMatrixSystem.hpp"
#include "../Resources/Managers/BufferManager.hpp"
#include <iostream>
#include <chrono>
#include "../../Core/GeneralManager.hpp"
#include "../GraphicsContexts.hpp"
#include "../Components/SwapChainComponent.hpp"
#include "../Components/BufferManagerComponent.hpp"
#include "../Components/CurrentFrameComponent.hpp"
#include "../Resources/Components/GlobalDSetComponent.hpp"
#include "../Resources/Components/ObjectDSetComponent.hpp"
#include "../Components/CameraComponent.hpp"
#include "../Components/GlobalTransformComponent.hpp"

void CameraMatrixSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "CameraMatrixSystem registered!" << std::endl;
}

void CameraMatrixSystem::onShutdown(GeneralManager& gm)
{
	std::cout << "CameraMatrixSystem shutdown!" << std::endl;
}

void CameraMatrixSystem::update(float deltaTime, GeneralManager& gm)
{
	CurrentFrameComponent* currentFrameComp = gm.getContextComponent<CurrentFrameContext, CurrentFrameComponent>();
	uint32_t currentFrame = currentFrameComp->currentFrame;
	BufferManager& bufferManager =
	    *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	SwapChain& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	CameraComponent* mainCamera = gm.getContextComponent<MainCameraContext, CameraComponent>();
	GlobalTransformComponent* mainCameraTransform = gm.getContextComponent<MainCameraContext, GlobalTransformComponent>();
	CameraComponent* sunCamera = gm.getContextComponent<LightCameraContext, CameraComponent>();
	GlobalTransformComponent* sunCameraTransform = gm.getContextComponent<LightCameraContext, GlobalTransformComponent>();
	GlobalDSetComponent* globalDSetComponent = gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();

	// === Sun ===
	glm::vec3 lightPos = glm::vec3(sunCameraTransform->globalPosition.x + mainCameraTransform->globalPosition.x,
	                               sunCameraTransform->globalPosition.y + mainCameraTransform->globalPosition.y,
	                               sunCameraTransform->globalPosition.z + mainCameraTransform->globalPosition.z);
	glm::vec3 lightTarget =
	    glm::vec3(mainCameraTransform->globalPosition.x, mainCameraTransform->globalPosition.y, mainCameraTransform->globalPosition.z);
	glm::mat4 lightView = glm::lookAt(lightPos, lightTarget, glm::vec3(0.0f, 1.0f, 0.0f));

	glm::mat4 lightProj = glm::orthoRH_ZO(-sunCamera->orthoSize, sunCamera->orthoSize, -sunCamera->orthoSize,
	                                      sunCamera->orthoSize, sunCamera->zNear, sunCamera->zFar);
	lightProj[1][1] *= -1; // Y-flip

	glm::mat4 lightSpaceMatrix = lightProj * lightView;

	SunStructue sunUbo{.lightSpaceMatrix = lightSpaceMatrix};
	memcpy(bufferManager.buffers[globalDSetComponent->sunCameraBuffers].bufferMapped[currentFrame], &sunUbo,
	       sizeof(sunUbo));

	// === Camera ===
	glm::mat4 view = mainCameraTransform->getViewMatrix();
	glm::mat4 proj = glm::perspective(glm::radians(mainCamera->fov),
	                                  static_cast<float>(swapChain.swapChainExtent.width) /
	                                      static_cast<float>(swapChain.swapChainExtent.height),
	                                  mainCamera->zNear, mainCamera->zFar);
	proj[1][1] *= -1; // Y-flip

	glm::mat4 cameraSpaceMatrix = proj * view;

	CameraStucture cameraUbo{.cameraSpaceMatrix = cameraSpaceMatrix};
	memcpy(bufferManager.buffers[globalDSetComponent->cameraBuffers].bufferMapped[currentFrame], &cameraUbo,
	       sizeof(cameraUbo));
}