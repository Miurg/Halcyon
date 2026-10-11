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

	batch.resize(renderAssetManager.meshCount());
	for (auto& group : batch)
	{
		group.clear();
	}

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
	baseTransformPerBatch.resize(batch.size());

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

	constexpr int kCategoryMap[] = {0, 0, 1, 1, 2, 2};

	drawInfo->segments.resize(kDrawVariantCount);
	uint32_t prevTotal = 0;

	for (uint32_t i = 0; i < kDrawVariantCount; ++i)
	{
		bool doubleSided = (kDrawVariants[i].cullMode == vk::CullModeFlagBits::eNone);
		writePrimitivesForPass(kCategoryMap[i], doubleSided, renderAssetManager, materialManager, primitivePtr,
		                       indirectBufferPtr, globalPrimitiveIndex, localPrimitiveIndex, globalCullIndex);
		drawInfo->segments[i] = {static_cast<uint32_t>(globalPrimitiveIndex - prevTotal), i};
		prevTotal = globalPrimitiveIndex;
	}

	drawInfo->totalDrawCount = globalPrimitiveIndex;
	drawInfo->totalObjectCount = localPrimitiveIndex;
}

void BufferUpdateSystem::writePrimitivesForPass(int categoryPass, bool isDoubleSidedPass,
                                                RenderAssetManager& renderAssetManager, MaterialManager& materialManager,
                                                ModelData* primitivePtr, IndirectDrawIndexedCommand* indirectBufferPtr,
                                                int& globalPrimitiveIndex, int& localPrimitiveIndex, int& globalCullIndex)
{
	IndirectDrawIndexedCommand currentDraw{};

	for (size_t b = 0; b < batch.size(); ++b)
	{
		const auto& agentsInBatch = batch[b];
		if (agentsInBatch.empty()) continue;

		MeshInfoComponent& meshBaseInfo = *agentsInBatch[0].meshInfo;
		MeshHandle meshIdx = meshBaseInfo.mesh;
		const auto& mesh = renderAssetManager.getMesh(meshIdx);
		int primitiveCount = mesh.primitives.size();

		for (int i = 0; i < primitiveCount; i++)
		{
			const auto& primitive = mesh.primitives[i];
			const auto& material = materialManager.getMaterial(primitive.materialIndex);
			int category = material.alphaMode; // 0=opaque, 1=mask, 2=blend
			bool isDoubleSided = (material.doubleSided == 1);

			if (categoryPass != category || isDoubleSidedPass != isDoubleSided) continue;

			// Write indirect draw command
			currentDraw.indexCount = primitive.indexCount;
			currentDraw.firstIndex = primitive.indexOffset;
			currentDraw.vertexOffset = primitive.vertexOffset;
			currentDraw.instanceCount = 0;
			currentDraw.firstInstance = globalCullIndex;
			indirectBufferPtr[globalPrimitiveIndex] = currentDraw;

			// Write per entity primitive data
			int currentEntityTransformIndex = baseTransformPerBatch[b];
			globalCullIndex += mesh.entitiesSubscribed;
			for (size_t entityIndex = 0; entityIndex < agentsInBatch.size(); ++entityIndex)
			{
				primitivePtr[localPrimitiveIndex].materialIndex = primitive.materialIndex.id;
				primitivePtr[localPrimitiveIndex].transformIndex = currentEntityTransformIndex;
				primitivePtr[localPrimitiveIndex].AABBMax = primitive.AABBMax;
				primitivePtr[localPrimitiveIndex].AABBMin = primitive.AABBMin;
				primitivePtr[localPrimitiveIndex].drawCommandIndex = globalPrimitiveIndex;

				localPrimitiveIndex++;
				currentEntityTransformIndex++;
			}
			globalPrimitiveIndex++;
		}
	}
}
