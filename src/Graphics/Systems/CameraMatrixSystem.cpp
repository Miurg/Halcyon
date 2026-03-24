#include "CameraMatrixSystem.hpp"
#include "../Resources/Managers/BufferManager.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <cmath>
#include "../GraphicsContexts.hpp"
#include "../Components/SwapChainComponent.hpp"
#include "../Components/BufferManagerComponent.hpp"
#include "../Components/CurrentFrameComponent.hpp"
#include "../Resources/Components/GlobalDSetComponent.hpp"
#include "../Components/CameraComponent.hpp"
#include "../Components/GlobalTransformComponent.hpp"
#include "../Components/DirectLightComponent.hpp"
#include "../Resources/ResourceStructures.hpp"

void CameraMatrixSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "CameraMatrixSystem registered!" << std::endl;
}

void CameraMatrixSystem::onShutdown(GeneralManager& gm)
{
	std::cout << "CameraMatrixSystem shutdown!" << std::endl;
}

void CameraMatrixSystem::update(GeneralManager& gm)
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
	DirectLightComponent* lightComponent = gm.getContextComponent<SunContext, DirectLightComponent>();

	// === Camera ===
	glm::mat4 view = mainCameraTransform->getViewMatrix();
	glm::mat4 proj = glm::perspectiveRH_ZO(glm::radians(mainCamera->fov),
	                                       static_cast<float>(swapChain.swapChainExtent.width) /
	                                           static_cast<float>(swapChain.swapChainExtent.height),
	                                       mainCamera->zFar, mainCamera->zNear);
	proj[1][1] *= -1; // Y-flip

	glm::mat4 cameraSpaceMatrix = proj * view;

	glm::mat4 transCamera = glm::transpose(cameraSpaceMatrix);
	glm::vec4 frustumPlanes[6];
	frustumPlanes[0] = transCamera[3] + transCamera[0]; // Left
	frustumPlanes[1] = transCamera[3] - transCamera[0]; // Right
	frustumPlanes[2] = transCamera[3] + transCamera[1]; // Bottom
	frustumPlanes[3] = transCamera[3] - transCamera[1]; // Top
	frustumPlanes[4] = transCamera[2];                  // Near
	frustumPlanes[5] = transCamera[3] - transCamera[2]; // Far

	for (int i = 0; i < 6; ++i)
	{
		float length = glm::length(glm::vec3(frustumPlanes[i]));
		frustumPlanes[i] /= length;
	}

	CameraStructure cameraUbo;
	cameraUbo.cameraSpaceMatrix = cameraSpaceMatrix;
	cameraUbo.viewMatrix = view;
	cameraUbo.projMatrix = proj;
	cameraUbo.invViewProj = glm::inverse(cameraSpaceMatrix);
	cameraUbo.cameraPositionAndPadding = glm::vec4(mainCameraTransform->globalPosition, 0.0f);
	for (int i = 0; i < 6; ++i) cameraUbo.frustumPlanes[i] = frustumPlanes[i];

	memcpy(bufferManager.buffers[globalDSetComponent->cameraBuffers.id].bufferMapped[currentFrame], &cameraUbo,
	       sizeof(cameraUbo));

	// === Sun (Shadows) ===

	// Calculate frustum corners in world space
	float aspect =
	    static_cast<float>(swapChain.swapChainExtent.width) / static_cast<float>(swapChain.swapChainExtent.height);
	float shadowZFar = std::min(mainCamera->zFar, lightComponent->shadowDistance);

	glm::mat4 shadowProj = glm::perspectiveRH_ZO(glm::radians(mainCamera->fov), aspect, mainCamera->zNear, shadowZFar);
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

	constexpr float RADIUS_ROUNDING_STEP = 4.0f; // Why do we need this again?
	radius = std::ceil(radius * RADIUS_ROUNDING_STEP) / RADIUS_ROUNDING_STEP; // Round radius to stabilize size slightly

	sunCamera->orthoSize = radius;

	// Calculate light view matrix
	glm::vec3 lightDir = glm::normalize(sunCameraTransform->front);
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

	glm::mat4 sunFrustumCamera = glm::transpose(lightSpaceMatrix);
	glm::vec4 sunFrustumPlanes[6];
	sunFrustumPlanes[0] = sunFrustumCamera[3] + sunFrustumCamera[0]; // Left
	sunFrustumPlanes[1] = sunFrustumCamera[3] - sunFrustumCamera[0]; // Right
	sunFrustumPlanes[2] = sunFrustumCamera[3] + sunFrustumCamera[1]; // Bottom
	sunFrustumPlanes[3] = sunFrustumCamera[3] - sunFrustumCamera[1]; // Top
	sunFrustumPlanes[4] = sunFrustumCamera[2];                // Near
	sunFrustumPlanes[5] = sunFrustumCamera[3] - sunFrustumCamera[2]; // Far

	for (int i = 0; i < 6; ++i)
	{
		float length = glm::length(glm::vec3(sunFrustumPlanes[i]));
		sunFrustumPlanes[i] /= length;
	}

	DirectLightStructure sunUbo{.lightSpaceMatrix = lightSpaceMatrix,
	                    .direction = glm::vec4(-lightDir, 1.0f),
	                    .color = lightComponent->color,
	                    .ambient = lightComponent->ambient,
	                    .shadowMapSize =
	                        glm::vec4(shadowMapWidth, shadowMapWidth, 1.0f / shadowMapWidth, 1.0f / shadowMapWidth)};
	for (int i = 0; i < 6; ++i) sunUbo.frustumPlanes[i] = sunFrustumPlanes[i];
	sunUbo.shadowCasterRange = lightComponent->shadowCasterRange;
	
	glm::vec2 lsMin(std::numeric_limits<float>::max());
	glm::vec2 lsMax(-std::numeric_limits<float>::max());

	for (const auto& corner : frustumCorners)
	{
		glm::vec4 ls = lightSpaceMatrix * corner;
		ls /= ls.w;
		lsMin.x = glm::min(lsMin.x, ls.x);
		lsMax.x = glm::max(lsMax.x, ls.x);
		lsMin.y = glm::min(lsMin.y, ls.y);
		lsMax.y = glm::max(lsMax.y, ls.y);
	}

	const float epsilon = 0.02f;
	sunUbo.cameraFrustumLightSpaceBounds =
	    glm::vec4(lsMin.x - epsilon, lsMax.x + epsilon, lsMin.y - epsilon, lsMax.y + epsilon);
	
	memcpy(bufferManager.buffers[globalDSetComponent->sunCameraBuffers.id].bufferMapped[currentFrame], &sunUbo,
	       sizeof(sunUbo));  
}