#include "BufferUpdateSystem.hpp"
#include "../Resources/Managers/BufferManager.hpp"
#include <iostream>
#include <chrono>
#include "../../Core/GeneralManager.hpp"
#include "../GraphicsContexts.hpp"
#include "../Components/SwapChainComponent.hpp"
#include "../Components/BufferManagerComponent.hpp"
#include "../Components/CurrentFrameComponent.hpp"
#include "../Resources/Components/ModelDSetComponent.hpp"
#include "../Resources/Components/MeshInfoComponent.hpp"
#include <map>
#include "../Resources/Managers/ModelManager.hpp"
#include "../Components/ModelManagerComponent.hpp"
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
	ModelManager& modelManager = *gm.getContextComponent<ModelManagerContext, ModelManagerComponent>()->modelManager;
	ModelDSetComponent* modelDSetComponent = gm.getContextComponent<MainDSetsContext, ModelDSetComponent>();

	std::vector<std::vector<Entity>> batch;
	batch.resize(modelManager.meshes.size());
	std::map<int, int> counts;
	for (const auto& entity : entities)
	{
		MeshInfoComponent& meshinfo = *gm.getComponent<MeshInfoComponent>(entity);
		batch[meshinfo.mesh].push_back(entity);
		counts[meshinfo.mesh]++;
	}
	for (const auto& [key, count] : counts)
	{
		modelManager.meshes[key].entitiesSubscribed = count;
	}

	// === Models ===
	auto* primitivePtr = static_cast<PrimitiveSctructure*>(
	    bufferManager.buffers[modelDSetComponent->primitiveBuffer].bufferMapped[currentFrame]);
	auto* transfromMeshPtr = static_cast<TransformStructure*>(
	    bufferManager.buffers[modelDSetComponent->transformBuffer].bufferMapped[currentFrame]);

	int globalTransformIndex = 0; 
	int globalPrimitiveIndex = 0; 

	for (const auto& entities : batch)
	{
		int baseTransformIndexForMesh = globalTransformIndex;

		for (const auto& entity : entities)
		{
			// Link global transform
			GlobalTransformComponent& transform = *gm.getComponent<GlobalTransformComponent>(entity);
			transfromMeshPtr[globalTransformIndex].model = transform.getGlobalModelMatrix();
			globalTransformIndex++;
		}

		if (entities.empty()) continue;

		// Get primitive count from mesh
		MeshInfoComponent& meshBaseInfo = *gm.getComponent<MeshInfoComponent>(entities[0]);
		int primitiveCount = modelManager.meshes[meshBaseInfo.mesh].primitives.size();

		for (int i = 0; i < primitiveCount; i++)
		{
			int currentEntityTransformIndex = baseTransformIndexForMesh;

			for (const auto& entity : entities)
			{
				MeshInfoComponent& meshinfo = *gm.getComponent<MeshInfoComponent>(entity);

				// Link texture index
				primitivePtr[globalPrimitiveIndex].textureIndex =
				    modelManager.meshes[meshinfo.mesh].primitives[i].textureIndex;

				// Link transform index
				primitivePtr[globalPrimitiveIndex].transformIndex = currentEntityTransformIndex;

				// Base color
				primitivePtr[globalPrimitiveIndex].baseColor =
				    modelManager.meshes[meshinfo.mesh].primitives[i].baseColorFactor;

				globalPrimitiveIndex++;
				currentEntityTransformIndex++;
			}
		}
	}
}