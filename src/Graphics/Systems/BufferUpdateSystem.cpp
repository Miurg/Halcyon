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
#include "../Resources/ResourceStructures.hpp"

void BufferUpdateSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "BufferUpdateSystem registered!" << std::endl;
}

void BufferUpdateSystem::onShutdown(GeneralManager& gm)
{
	std::cout << "BufferUpdateSystem shutdown!" << std::endl;
}

void BufferUpdateSystem::onEntitySubscribed(Entity entity, GeneralManager& gm)
{
	GlobalTransformComponent* transform = gm.getComponent<GlobalTransformComponent>(entity);
	MeshInfoComponent* meshInfo = gm.getComponent<MeshInfoComponent>(entity);

	if (transform && meshInfo)
	{
		_agents.push_back({entity, transform, meshInfo});
	}
}

void BufferUpdateSystem::onEntityUnsubscribed(Entity entity, GeneralManager& gm)
{
	auto it =
	    std::remove_if(_agents.begin(), _agents.end(), [entity](const Agent& agent) { return agent.entity == entity; });
	_agents.erase(it, _agents.end());
}

void BufferUpdateSystem::update(float deltaTime, GeneralManager& gm)
{
	CurrentFrameComponent* currentFrameComp = gm.getContextComponent<CurrentFrameContext, CurrentFrameComponent>();
	uint32_t currentFrame = currentFrameComp->currentFrame;
	SwapChain& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	BufferManager& bufferManager =
	    *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	ModelManager& modelManager = *gm.getContextComponent<ModelManagerContext, ModelManagerComponent>()->modelManager;
	ModelDSetComponent* objectDSetComponent = gm.getContextComponent<MainDSetsContext, ModelDSetComponent>();

	std::vector<std::vector<Agent>> batch;
	batch.resize(modelManager.meshes.size());

	for (auto& agent : _agents)
	{
		batch[agent.meshInfo->mesh].push_back(agent);
	}

	for (size_t i = 0; i < batch.size(); ++i)
	{
		if (!batch[i].empty())
		{
			modelManager.meshes[i].entitiesSubscribed = batch[i].size();
		}
	}

	// === Models ===
	auto* primitivePtr = static_cast<PrimitiveSctructure*>(
	    bufferManager.buffers[objectDSetComponent->primitiveBuffer.id].bufferMapped[currentFrame]);
	auto* transfromMeshPtr = static_cast<TransformStructure*>(
	    bufferManager.buffers[objectDSetComponent->transformBuffer.id].bufferMapped[currentFrame]);
	auto* indirectBufferPtr = static_cast<IndirectDrawStructure*>(
	    bufferManager.buffers[objectDSetComponent->indirectDrawBuffer.id].bufferMapped[currentFrame]);

	int globalTransformIndex = 0;
	int globalPrimitiveIndex = 0;
	int localPrimitiveIndex = 0;
	int globalCullIndex = 0;
	IndirectDrawStructure currentDraw{};

	for (const auto& agentsInBatch : batch)
	{
		int baseTransformIndexForMesh = globalTransformIndex;

		for (const auto& agent : agentsInBatch)
		{
			// Link global transform
			transfromMeshPtr[globalTransformIndex].model = agent.transform->getGlobalModelMatrix();
			globalTransformIndex++;
		}

		if (agentsInBatch.empty()) continue;

		// Get primitive count from mesh
		// Optimized access via first agent in batch
		MeshInfoComponent& meshBaseInfo = *agentsInBatch[0].meshInfo;
		int primitiveCount = modelManager.meshes[meshBaseInfo.mesh].primitives.size();

		for (int i = 0; i < primitiveCount; i++)
		{
			currentDraw.indexCount = modelManager.meshes[meshBaseInfo.mesh].primitives[i].indexCount;
			currentDraw.firstIndex = modelManager.meshes[meshBaseInfo.mesh].primitives[i].indexOffset;
			currentDraw.vertexOffset = modelManager.meshes[meshBaseInfo.mesh].primitives[i].vertexOffset;
			currentDraw.instanceCount = 0;
			currentDraw.firstInstance = globalCullIndex;
			indirectBufferPtr[globalPrimitiveIndex] = currentDraw;

			int currentEntityTransformIndex = baseTransformIndexForMesh;
			globalCullIndex += modelManager.meshes[meshBaseInfo.mesh].entitiesSubscribed;
			for (const auto& agent : agentsInBatch)
			{
				int meshIndex = agent.meshInfo->mesh;

				// Link texture index
				primitivePtr[localPrimitiveIndex].materialIndex = modelManager.meshes[meshIndex].primitives[i].materialIndex;

				// Link transform index
				primitivePtr[localPrimitiveIndex].transformIndex = currentEntityTransformIndex;

				primitivePtr[localPrimitiveIndex].AABBMax = modelManager.meshes[meshIndex].primitives[i].AABBMax;
				primitivePtr[localPrimitiveIndex].AABBMin = modelManager.meshes[meshIndex].primitives[i].AABBMin;

				primitivePtr[localPrimitiveIndex].drawCommandIndex = globalPrimitiveIndex;

				localPrimitiveIndex++;
				currentEntityTransformIndex++;
			}
			globalPrimitiveIndex++;
		}
	}
}