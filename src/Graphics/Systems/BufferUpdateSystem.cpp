#include "BufferUpdateSystem.hpp"
#include "../Components/TransformComponent.hpp"
#include "../Resources/Managers/BufferManager.hpp"
#include <iostream>
#include <chrono>
#include "../../Core/GeneralManager.hpp"
#include "../GraphicsContexts.hpp"
#include "../Components/SwapChainComponent.hpp"
#include "../Components/BufferManagerComponent.hpp"
#include "../Components/CurrentFrameComponent.hpp"
#include "../Resources/Components/ObjectDSetComponent.hpp"
#include "../Resources/Components/MeshInfoComponent.hpp"
#include <map>
void BufferUpdateSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "BufferUpdateSystem registered!" << std::endl;
}

void BufferUpdateSystem::onShutdown(GeneralManager& gm)
{
	std::cout << "BufferUpdateSystem shutdown!" << std::endl;
}

void BufferUpdateSystem::update(float deltaTime, GeneralManager& gm)
{
	CurrentFrameComponent* currentFrameComp = gm.getContextComponent<CurrentFrameContext, CurrentFrameComponent>();
	uint32_t currentFrame = currentFrameComp->currentFrame;
	SwapChain& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	BufferManager& bufferManager =
	    *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	ObjectDSetComponent* objectDSetComponent = gm.getContextComponent<MainDSetsContext, ObjectDSetComponent>();

	std::vector<std::vector<Entity>> batch;
	batch.resize(bufferManager.meshes.size());
	std::map<int, int> counts;
	for (const auto& entity : entities)
	{
		MeshInfoComponent& meshinfo = *gm.getComponent<MeshInfoComponent>(entity);
		batch[meshinfo.mesh].push_back(entity);
		counts[meshinfo.mesh]++;
	}
	for (const auto& [key, count] : counts)
	{
		bufferManager.meshes[key].entitiesSubscribed = count;
	}

	// === Models ===
	auto* dstPtr = static_cast<ModelSctructure*>(
	    bufferManager.buffers[objectDSetComponent->storageBuffer].bufferMapped[currentFrame]);

	int temp = 0;
	for (const auto& entities : batch)
	{
		for (const auto& entity : entities)
		{
			TransformComponent& transform = *gm.getComponent<TransformComponent>(entity);
			const glm::mat4 model = transform.getModelMatrix();
			dstPtr[temp].model = model;

			TextureInfoComponent& texture = *gm.getComponent<TextureInfoComponent>(entity);
			dstPtr[temp].textureIndex = texture.textureIndex;
			temp++;
		}	
	}
}