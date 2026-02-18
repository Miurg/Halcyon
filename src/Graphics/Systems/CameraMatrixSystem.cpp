#include "CameraMatrixSystem.hpp"
#include "../Resources/Managers/BufferManager.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <cmath>
#include "../../Core/GeneralManager.hpp"
#include "../GraphicsContexts.hpp"
#include "../Components/SwapChainComponent.hpp"
#include "../Components/BufferManagerComponent.hpp"
#include "../Components/CurrentFrameComponent.hpp"
#include "../Resources/Components/GlobalDSetComponent.hpp"
#include "../Components/CameraComponent.hpp"
#include "../Components/GlobalTransformComponent.hpp"
#include "../Components/LightComponent.hpp"
#include "../Resources/ResourceStructures.hpp"

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
	GlobalTransformComponent* mainCameraTransform =
	    gm.getContextComponent<MainCameraContext, GlobalTransformComponent>();
	CameraComponent* sunCamera = gm.getContextComponent<SunContext, CameraComponent>();
	GlobalTransformComponent* sunCameraTransform = gm.getContextComponent<SunContext, GlobalTransformComponent>();
	GlobalDSetComponent* globalDSetComponent = gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	LightComponent* lightComponent = gm.getContextComponent<SunContext, LightComponent>();

	// === Camera ===
	glm::mat4 view = mainCameraTransform->getViewMatrix();
	glm::mat4 proj = glm::perspective(glm::radians(mainCamera->fov),
	                                  static_cast<float>(swapChain.swapChainExtent.width) /
	                                      static_cast<float>(swapChain.swapChainExtent.height),
	                                  mainCamera->zNear, mainCamera->zFar);
	proj[1][1] *= -1; // Y-flip

	glm::mat4 cameraSpaceMatrix = proj * view;

	CameraStructure cameraUbo{.cameraSpaceMatrix = cameraSpaceMatrix};
	memcpy(bufferManager.buffers[globalDSetComponent->cameraBuffers.id].bufferMapped[currentFrame], &cameraUbo,
	       sizeof(cameraUbo));

	// === Sun (Shadows) ===

	// Calculate frustum corners in world space
	float aspect =
	    static_cast<float>(swapChain.swapChainExtent.width) / static_cast<float>(swapChain.swapChainExtent.height);
	float shadowZFar = std::min(mainCamera->zFar, lightComponent->shadowDistance);

	glm::mat4 shadowProj = glm::perspective(glm::radians(mainCamera->fov), aspect, mainCamera->zNear, shadowZFar);
	shadowProj[1][1] *= -1; // Y-flip

	glm::mat4 shadowCamSpaceMatrix = shadowProj * view;
	glm::mat4 invCam = glm::inverse(shadowCamSpaceMatrix);

	std::vector<glm::vec4> frustumCorners;
	for (unsigned int x = 0; x < 2; ++x)
	{
		for (unsigned int y = 0; y < 2; ++y)
		{
			for (unsigned int z = 0; z < 2; ++z)
			{
				glm::vec4 pt = invCam * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f,
				                                  static_cast<float>(z), // 0 to 1 for Z (Vulkan)
				                                  1.0f);
				frustumCorners.push_back(pt / pt.w);
			}
		}
	}

	// Frustum Center
	glm::vec3 center = glm::vec3(0, 0, 0);
	for (const auto& v : frustumCorners)
	{
		center += glm::vec3(v);
	}
	center /= frustumCorners.size();

	// Bounding sphere radius
	float radius = 0.0f;
	for (const auto& v : frustumCorners)
	{
		float distance = glm::length(glm::vec3(v) - center);
		radius = glm::max(radius, distance);
	}

	constexpr float RADIUS_ROUNDING_STEP = 16.0f;
	radius = std::ceil(radius * RADIUS_ROUNDING_STEP) / RADIUS_ROUNDING_STEP; // Round radius to stabilize size slightly

	sunCamera->orthoSize = radius;

	// Calculate light view matrix
	glm::vec3 lightDir = glm::normalize(-glm::vec3(sunCameraTransform->globalPosition));
	float zOffset = radius + lightComponent->shadowCasterRange;
	glm::vec3 lightPos = center - lightDir * zOffset;
	glm::mat4 lightView = glm::lookAt(lightPos, center, glm::vec3(0.0f, 1.0f, 0.0f));

	// Texel snapping
	float zFarRange = zOffset + radius;

	glm::mat4 lightProj = glm::orthoRH_ZO(-radius, radius, -radius, radius, zFarRange, 0.0f);
	lightProj[1][1] *= -1; // Y-flip

	glm::mat4 shadowMatrix = lightProj * lightView;
	float shadowMapWidth = static_cast<float>(lightComponent->sizeX);

	// Transform world origin to shadow space
	glm::vec4 shadowOrigin = shadowMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	shadowOrigin = shadowOrigin * (shadowMapWidth / 2.0f);

	glm::vec4 roundedOrigin = glm::round(shadowOrigin);
	glm::vec4 roundOffset = roundedOrigin - shadowOrigin;
	roundOffset = roundOffset * (2.0f / shadowMapWidth);

	lightProj[3][0] += roundOffset.x;
	lightProj[3][1] += roundOffset.y;

	glm::mat4 lightSpaceMatrix = lightProj * lightView;

	SunStructure sunUbo{
	    .lightSpaceMatrix = lightSpaceMatrix,
	    .direction = glm::vec4(-lightDir, 1.0f),
	    .color = lightComponent->color,
	    .ambient = lightComponent->ambient,
	};
	memcpy(bufferManager.buffers[globalDSetComponent->sunCameraBuffers.id].bufferMapped[currentFrame], &sunUbo,
	       sizeof(sunUbo));
}