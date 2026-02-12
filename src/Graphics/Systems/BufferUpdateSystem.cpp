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
#include "../Resources/Components/FrustumDSetComponent.hpp"
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
	auto it = std::remove_if(_agents.begin(), _agents.end(),
	                         [entity](const Agent& agent) { return agent.entity == entity; });
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
	ModelDSetComponent* modelDSetComponent = gm.getContextComponent<MainDSetsContext, ModelDSetComponent>();
	FrustumDSetComponent* frustumDSetComponent = gm.getContextComponent<MainDSetsContext, FrustumDSetComponent>();

	std::vector<std::vector<Agent*>> batch;
	batch.resize(modelManager.meshes.size());
	std::map<int, int> counts;

	for (auto& agent : _agents)
	{
		batch[agent.meshInfo->mesh].push_back(&agent);
		counts[agent.meshInfo->mesh]++;
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
	auto* indirectBufferPtr = static_cast<IndirectDrawStructure*>(
	    bufferManager.buffers[frustumDSetComponent->indirectDrawBuffer].bufferMapped[currentFrame]);

	int globalTransformIndex = 0;
	int globalPrimitiveIndex = 0;
	int localPrimitiveIndex = 0;
	int globalCullIndex = 0;
	IndirectDrawStructure currentDraw{};
	
	for (const auto& agentsInBatch : batch)
	{
		int baseTransformIndexForMesh = globalTransformIndex;

		for (const auto* agent : agentsInBatch)
		{
			// Link global transform
			transfromMeshPtr[globalTransformIndex].model = agent->transform->getGlobalModelMatrix();
			globalTransformIndex++;
		}

		if (agentsInBatch.empty()) continue;

		// Get primitive count from mesh
		// Optimized access via first agent in batch
		MeshInfoComponent& meshBaseInfo = *agentsInBatch[0]->meshInfo;
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
			for (const auto* agent : agentsInBatch)
			{
				MeshInfoComponent& meshinfo = *agent->meshInfo;

				// Link texture index
				primitivePtr[localPrimitiveIndex].textureIndex =
				    modelManager.meshes[meshinfo.mesh].primitives[i].textureIndex;

				// Link transform index
				primitivePtr[localPrimitiveIndex].transformIndex = currentEntityTransformIndex;

				// Base color
				primitivePtr[localPrimitiveIndex].baseColor =
				    modelManager.meshes[meshinfo.mesh].primitives[i].baseColorFactor;

				primitivePtr[localPrimitiveIndex].AABBMax = modelManager.meshes[meshinfo.mesh].primitives[i].AABBMax;
				primitivePtr[localPrimitiveIndex].AABBMin = modelManager.meshes[meshinfo.mesh].primitives[i].AABBMin;

				primitivePtr[localPrimitiveIndex].drawCommandIndex = globalPrimitiveIndex;

				localPrimitiveIndex++;
				currentEntityTransformIndex++;
			}
			globalPrimitiveIndex++;
		}
	}
}