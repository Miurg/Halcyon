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
#include "GraphicsCore/Resources/Managers/RenderAssetManager.hpp"
#include "GraphicsCore/Components/RenderAssetManagerComponent.hpp"
#include "GraphicsCore/Components/TextureManagerComponent.hpp"
#include "GraphicsCore/Resources/Managers/TextureManager.hpp"
#include "GraphicsCore/Components/MaterialManagerComponent.hpp"
#include "GraphicsCore/Resources/Managers/MaterialManager.hpp"
#include "Shared/GpuStructs.h"

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
	RenderAssetManager& renderAssetManager =
	    *gm.getContextComponent<RenderAssetManagerContext, RenderAssetManagerComponent>()->renderAssetManager;
	MaterialManager& materialManager =
	    *gm.getContextComponent<MaterialManagerContext, MaterialManagerComponent>()->materialManager;
	ModelDSetComponent* objectDSetComponent = gm.getContextComponent<MainDSetsContext, ModelDSetComponent>();
	DrawInfoComponent* drawInfo = gm.getContextComponent<CurrentFrameContext, DrawInfoComponent>();

	struct Agent
	{
		GlobalTransformComponent* transform;
		MeshInfoComponent* meshInfo;
	};

	std::vector<std::vector<Agent>> batch;
	batch.resize(renderAssetManager.meshCount());

	forEachSubscribedEntity(gm, [&](Orhescyon::Entity, GlobalTransformComponent& transform, MeshInfoComponent& meshInfo)
	                        { batch[meshInfo.mesh.id].push_back({&transform, &meshInfo}); });

	for (size_t i = 0; i < batch.size(); ++i)
	{
		if (!batch[i].empty())
		{
			renderAssetManager.getMesh(MeshHandle{static_cast<int>(i)}).entitiesSubscribed = batch[i].size();
		}
	}

	auto* primitivePtr = bufferManager.getMapped<ModelData>(objectDSetComponent->primitiveBuffer, currentFrame);
	auto* transfromMeshPtr =
	    bufferManager.getMapped<TransformData>(objectDSetComponent->transformBuffer, currentFrame);
	auto* indirectBufferPtr =
	    bufferManager.getMapped<IndirectDrawIndexedCommand>(objectDSetComponent->indirectDrawBuffer, currentFrame);

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
	IndirectDrawIndexedCommand currentDraw{};

	auto writePrimitivesForPass = [&](int categoryPass, bool isDoubleSidedPass)
	{
		for (size_t b = 0; b < batch.size(); ++b)
		{
			const auto& agentsInBatch = batch[b];
			if (agentsInBatch.empty()) continue;

			MeshInfoComponent& meshBaseInfo = *agentsInBatch[0].meshInfo;
			MeshHandle meshIdx = meshBaseInfo.mesh;
			int primitiveCount = renderAssetManager.getMesh(meshIdx).primitives.size();

			for (int i = 0; i < primitiveCount; i++)
			{
				MaterialHandle matIdx = renderAssetManager.getMesh(meshIdx).primitives[i].materialIndex;
				int category = materialManager.getMaterial(matIdx).alphaMode; // 0=opaque, 1=mask, 2=blend
				bool isDoubleSided = (materialManager.getMaterial(matIdx).doubleSided == 1);

				if (categoryPass != category || isDoubleSidedPass != isDoubleSided) continue;

				// Write indirect draw command
				currentDraw.indexCount = renderAssetManager.getMesh(meshIdx).primitives[i].indexCount;
				currentDraw.firstIndex = renderAssetManager.getMesh(meshIdx).primitives[i].indexOffset;
				currentDraw.vertexOffset = renderAssetManager.getMesh(meshIdx).primitives[i].vertexOffset;
				currentDraw.instanceCount = 0;
				currentDraw.firstInstance = globalCullIndex;
				indirectBufferPtr[globalPrimitiveIndex] = currentDraw;

				// Write per entity primitive data
				int currentEntityTransformIndex = baseTransformPerBatch[b];
				globalCullIndex += renderAssetManager.getMesh(meshIdx).entitiesSubscribed;
				for (const auto& agent : agentsInBatch)
				{
					MeshHandle agentMeshIndex = agent.meshInfo->mesh;

					primitivePtr[localPrimitiveIndex].materialIndex =
					    renderAssetManager.getMesh(agentMeshIndex).primitives[i].materialIndex.id;
					primitivePtr[localPrimitiveIndex].transformIndex = currentEntityTransformIndex;
					primitivePtr[localPrimitiveIndex].AABBMax =
					    renderAssetManager.getMesh(agentMeshIndex).primitives[i].AABBMax;
					primitivePtr[localPrimitiveIndex].AABBMin =
					    renderAssetManager.getMesh(agentMeshIndex).primitives[i].AABBMin;
					primitivePtr[localPrimitiveIndex].drawCommandIndex = globalPrimitiveIndex;

					localPrimitiveIndex++;
					currentEntityTransformIndex++;
				}
				globalPrimitiveIndex++;
			}
		}
	};

	constexpr int kCategoryMap[] = {0, 0, 1, 1, 2, 2};

	drawInfo->segments.resize(kDrawVariantCount);
	uint32_t prevTotal = 0;

	for (uint32_t i = 0; i < kDrawVariantCount; ++i)
	{
		bool doubleSided = (kDrawVariants[i].cullMode == vk::CullModeFlagBits::eNone);
		writePrimitivesForPass(kCategoryMap[i], doubleSided);
		drawInfo->segments[i] = {static_cast<uint32_t>(globalPrimitiveIndex - prevTotal), i};
		prevTotal = globalPrimitiveIndex;
	}

	drawInfo->totalDrawCount = globalPrimitiveIndex;
	drawInfo->totalObjectCount = localPrimitiveIndex;
}
