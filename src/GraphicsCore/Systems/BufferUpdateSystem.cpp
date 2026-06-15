#include "GraphicsCore/Systems/BufferUpdateSystem.hpp"
#include "GraphicsCore/Resources/Managers/BufferManager.hpp"
#include <iostream>
#include <chrono>
#include "GraphicsCore/GraphicsContexts.hpp"
#include "GraphicsCore/Components/SwapChainComponent.hpp"
#include "GraphicsCore/Components/BufferManagerComponent.hpp"
#include "GraphicsCore/Components/CurrentFrameComponent.hpp"
#include "GraphicsCore/Components/DrawInfoComponent.hpp"
#include "GraphicsCore/Resources/Components/ModelDSetComponent.hpp"
#include "GraphicsCore/Resources/Components/MeshInfoComponent.hpp"
#include <map>
#include "GraphicsCore/Resources/Managers/ModelManager.hpp"
#include "GraphicsCore/Components/ModelManagerComponent.hpp"
#include "GraphicsCore/Components/TextureManagerComponent.hpp"
#include "GraphicsCore/Resources/Managers/TextureManager.hpp"
#include "GraphicsCore/Resources/ResourceStructures.hpp"

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

void BufferUpdateSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "BufferUpdateSystem registered!" << std::endl;
}

void BufferUpdateSystem::onShutdown(GeneralManager& gm)
{
	std::cout << "BufferUpdateSystem shutdown!" << std::endl;
}

void BufferUpdateSystem::onEntitySubscribed(Orhescyon::Entity entity, GeneralManager& gm)
{
	GlobalTransformComponent* transform = gm.getComponent<GlobalTransformComponent>(entity);
	MeshInfoComponent* meshInfo = gm.getComponent<MeshInfoComponent>(entity);

	if (transform && meshInfo)
	{
		_agents.push_back({entity, transform, meshInfo});
	}
}

void BufferUpdateSystem::onEntityUnsubscribed(Orhescyon::Entity entity, GeneralManager& gm)
{
	auto it =
	    std::remove_if(_agents.begin(), _agents.end(), [entity](const Agent& agent) { return agent.entity == entity; });
	_agents.erase(it, _agents.end());
}

void BufferUpdateSystem::update(GeneralManager& gm)
{
#ifdef TRACY_ENABLE
	ZoneScopedN("BufferUpdateSystem");
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

	auto* primitivePtr = static_cast<PrimitiveSctructure*>(
	    bufferManager.buffers[objectDSetComponent->primitiveBuffer.id].bufferMapped[currentFrame]);
	auto* transfromMeshPtr = static_cast<TransformStructure*>(
	    bufferManager.buffers[objectDSetComponent->transformBuffer.id].bufferMapped[currentFrame]);
	auto* indirectBufferPtr = static_cast<IndirectDrawStructure*>(
	    bufferManager.buffers[objectDSetComponent->indirectDrawBuffer.id].bufferMapped[currentFrame]);

	// same for both passes
	int globalTransformIndex = 0;
	std::vector<int> baseTransformPerBatch(batch.size(), 0);

	for (size_t b = 0; b < batch.size(); ++b)
	{
		baseTransformPerBatch[b] = globalTransformIndex;
		for (const auto& agent : batch[b])
		{
			transfromMeshPtr[globalTransformIndex].model = agent.transform->getGlobalModelMatrix();
			globalTransformIndex++;
		}
	}

	int globalPrimitiveIndex = 0;
	int localPrimitiveIndex = 0;
	int globalCullIndex = 0;
	IndirectDrawStructure currentDraw{};

	auto writePrimitivesForPass = [&](int categoryPass, bool isDoubleSidedPass)
	{
		for (size_t b = 0; b < batch.size(); ++b)
		{
			const auto& agentsInBatch = batch[b];
			if (agentsInBatch.empty()) continue;

			MeshInfoComponent& meshBaseInfo = *agentsInBatch[0].meshInfo;
			int meshIdx = meshBaseInfo.mesh;
			int primitiveCount = modelManager.meshes[meshIdx].primitives.size();

			for (int i = 0; i < primitiveCount; i++)
			{
				uint32_t matIdx = modelManager.meshes[meshIdx].primitives[i].materialIndex;
				int category = textureManager.materials[matIdx].alphaMode; // 0=opaque, 1=mask, 2=blend
				bool isDoubleSided = (textureManager.materials[matIdx].doubleSided == 1);

				if (categoryPass != category || isDoubleSidedPass != isDoubleSided) continue;

				// Write indirect draw command
				currentDraw.indexCount = modelManager.meshes[meshIdx].primitives[i].indexCount;
				currentDraw.firstIndex = modelManager.meshes[meshIdx].primitives[i].indexOffset;
				currentDraw.vertexOffset = modelManager.meshes[meshIdx].primitives[i].vertexOffset;
				currentDraw.instanceCount = 0;
				currentDraw.firstInstance = globalCullIndex;
				indirectBufferPtr[globalPrimitiveIndex] = currentDraw;

				// Write per entity primitive data
				int currentEntityTransformIndex = baseTransformPerBatch[b];
				globalCullIndex += modelManager.meshes[meshIdx].entitiesSubscribed;
				for (const auto& agent : agentsInBatch)
				{
					int agentMeshIndex = agent.meshInfo->mesh;

					primitivePtr[localPrimitiveIndex].materialIndex =
					    modelManager.meshes[agentMeshIndex].primitives[i].materialIndex;
					primitivePtr[localPrimitiveIndex].transformIndex = currentEntityTransformIndex;
					primitivePtr[localPrimitiveIndex].AABBMax = modelManager.meshes[agentMeshIndex].primitives[i].AABBMax;
					primitivePtr[localPrimitiveIndex].AABBMin = modelManager.meshes[agentMeshIndex].primitives[i].AABBMin;
					primitivePtr[localPrimitiveIndex].drawCommandIndex = globalPrimitiveIndex;

					localPrimitiveIndex++;
					currentEntityTransformIndex++;
				}
				globalPrimitiveIndex++;
			}
		}
	}; 

	uint32_t prevTotal = 0;

	writePrimitivesForPass(0, false); // Opaque Single-Sided
	uint32_t opaqueSingleCount = globalPrimitiveIndex - prevTotal;
	prevTotal = globalPrimitiveIndex;

	writePrimitivesForPass(0, true); // Opaque Double-Sided
	uint32_t opaqueDoubleCount = globalPrimitiveIndex - prevTotal;
	prevTotal = globalPrimitiveIndex;

	writePrimitivesForPass(1, false); // Mask Single-Sided
	uint32_t maskSingleCount = globalPrimitiveIndex - prevTotal;
	prevTotal = globalPrimitiveIndex;

	writePrimitivesForPass(1, true); // Mask Double-Sided
	uint32_t maskDoubleCount = globalPrimitiveIndex - prevTotal;
	prevTotal = globalPrimitiveIndex;

	writePrimitivesForPass(2, false); // Blend Single-Sided
	uint32_t blendSingleCount = globalPrimitiveIndex - prevTotal;
	prevTotal = globalPrimitiveIndex;

	writePrimitivesForPass(2, true); // Blend Double-Sided
	uint32_t blendDoubleCount = globalPrimitiveIndex - prevTotal;

	drawInfo->opaqueSingleCount = opaqueSingleCount;
	drawInfo->opaqueDoubleCount = opaqueDoubleCount;
	drawInfo->maskSingleCount = maskSingleCount;
	drawInfo->maskDoubleCount = maskDoubleCount;
	drawInfo->blendSingleCount = blendSingleCount;
	drawInfo->blendDoubleCount = blendDoubleCount;

	drawInfo->totalDrawCount = globalPrimitiveIndex;
	drawInfo->totalObjectCount = localPrimitiveIndex;
}