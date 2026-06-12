#include "LightUpdateSystem.hpp"
#include "../Resources/Managers/BufferManager.hpp"
#include <iostream>
#include <chrono>
#include "../GraphicsContexts.hpp"
#include "../Components/SwapChainComponent.hpp"
#include "../Components/BufferManagerComponent.hpp"
#include "../Components/CurrentFrameComponent.hpp"
#include "../Components/DrawInfoComponent.hpp"
#include "../Resources/Components/ModelDSetComponent.hpp"
#include "../Resources/Components/MeshInfoComponent.hpp"
#include <map>
#include "../Resources/Managers/ModelManager.hpp"
#include "../Components/ModelManagerComponent.hpp"
#include "../Components/TextureManagerComponent.hpp"
#include "../Resources/Managers/TextureManager.hpp"
#include "../Resources/ResourceStructures.hpp"
#include "../Resources/Components/GlobalDSetComponent.hpp"

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

void LightUpdateSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "LightUpdateSystem registered!" << std::endl;
}

void LightUpdateSystem::onShutdown(GeneralManager& gm)
{
	std::cout << "LightUpdateSystem shutdown!" << std::endl;
}

void LightUpdateSystem::onEntitySubscribed(Orhescyon::Entity entity, GeneralManager& gm)
{
	GlobalTransformComponent* transform = gm.getComponent<GlobalTransformComponent>(entity);
	PointLightComponent* lightInfo = gm.getComponent<PointLightComponent>(entity);

	if (transform && lightInfo)
	{
		_agents.push_back({entity, transform, lightInfo});
	}
}

void LightUpdateSystem::onEntityUnsubscribed(Orhescyon::Entity entity, GeneralManager& gm)
{
	auto it =
	    std::remove_if(_agents.begin(), _agents.end(), [entity](const Agent& agent) { return agent.entity == entity; });
	_agents.erase(it, _agents.end());
}

void LightUpdateSystem::update(GeneralManager& gm)
{
#ifdef TRACY_ENABLE
	ZoneScopedN("LightUpdateSystem");
#endif

	CurrentFrameComponent* currentFrameComp = gm.getContextComponent<CurrentFrameContext, CurrentFrameComponent>();
	uint32_t currentFrame = currentFrameComp->currentFrame;
	SwapChain& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	BufferManager& bufferManager =
	    *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	ModelManager& modelManager = *gm.getContextComponent<ModelManagerContext, ModelManagerComponent>()->modelManager;
	TextureManager& textureManager =
	    *gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	ModelDSetComponent* objectDSetComponent = gm.getContextComponent<MainDSetsContext, ModelDSetComponent>();
	DrawInfoComponent* drawInfo = gm.getContextComponent<CurrentFrameContext, DrawInfoComponent>();
	GlobalDSetComponent* globalDSetComponent = gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();


	auto* spotLightPtr = static_cast<PointLightStructure*>(
	    bufferManager.buffers[globalDSetComponent->pointLightBuffers.id].bufferMapped[currentFrame]);
	for (int i = 0; i < _agents.size(); ++i)
	{
		spotLightPtr[i].color = _agents[i].lightInfo->color;
		spotLightPtr[i].direction = glm::normalize(_agents[i].lightInfo->direction);
		spotLightPtr[i].intensity = _agents[i].lightInfo->intensity;
		spotLightPtr[i].radius = _agents[i].lightInfo->radius;
		spotLightPtr[i].innerConeAngle = glm::cos(glm::radians(_agents[i].lightInfo->innerConeAngle));
		spotLightPtr[i].outerConeAngle = glm::cos(glm::radians(_agents[i].lightInfo->outerConeAngle));
		spotLightPtr[i].type = _agents[i].lightInfo->type;

		spotLightPtr[i].position = _agents[i].transform->getGlobalPosition();
	}

	auto* pointLightCountPtr = static_cast<uint32_t*>(
	    bufferManager.buffers[globalDSetComponent->pointLightCountBuffer.id].bufferMapped[currentFrame]);
	pointLightCountPtr[0] = _agents.size();
}