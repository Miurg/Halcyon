#include "RenderSystem.hpp"
#include <iostream>
#include "../../Core/GeneralManager.hpp"
#include "../GraphicsContexts.hpp"
#include "../Factories/CommandBufferFactory.hpp"
#include "../Components/PipelineHandlerComponent.hpp"
#include "../Components/SwapChainComponent.hpp"
#include "../Components/BufferManagerComponent.hpp"
#include "../FrameData.hpp"
#include "../Components/FrameDataComponent.hpp"
#include "../Components/CurrentFrameComponent.hpp"
#include "../Components/DescriptorManagerComponent.hpp"
#include "../Components/FrameImageComponent.hpp"

void RenderSystem::processEntity(Entity entity, GeneralManager& manager, float dt) {}

void RenderSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "RenderSystem registered!" << std::endl;
}

void RenderSystem::onShutdown(GeneralManager& gm)
{
	std::cout << "RenderSystem shutdown!" << std::endl;
}

void RenderSystem::update(float deltaTime, GeneralManager& gm, const std::vector<Entity>& entities)
{
	SwapChain& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	PipelineHandler& pipelineHandler =
	    *gm.getContextComponent<MainSignatureContext, PipelineHandlerComponent>()->pipelineHandler;
	BufferManager& bufferManager =
	    *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	std::vector<FrameData>& framesData =
	    *gm.getContextComponent<MainFrameDataContext, FrameDataComponent>()->frameDataArray;
	CurrentFrameComponent* currentFrameComp = gm.getContextComponent<CurrentFrameContext, CurrentFrameComponent>();
	CameraComponent* mainCamera = gm.getContextComponent<MainCameraContext, CameraComponent>();
	LightComponent* lightTexture = gm.getContextComponent<LightCameraContext, LightComponent>();
	BindlessTextureDSetComponent* materialDSetComponent =
	    gm.getContextComponent<MainDSetsContext, BindlessTextureDSetComponent>();
	DescriptorManagerComponent* dManager =
	    gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	GlobalDSetComponent* globalDSetComponent = gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	ObjectDSetComponent* objectDSetComponent = gm.getContextComponent<MainDSetsContext, ObjectDSetComponent>();
	
	uint32_t imageIndex = gm.getContextComponent<FrameImageContext, FrameImageComponent>()->imageIndex;
	
	std::vector<int> textureInfo;
	std::vector<MeshInfoComponent*> meshInfos;
	for (int i = 0; i < entities.size(); i++)
	{
		meshInfos.push_back(gm.getComponent<MeshInfoComponent>(entities[i]));
		auto textureInfoComponent = gm.getComponent<TextureInfoComponent>(entities[i]);
		textureInfo.push_back(textureInfoComponent->textureIndex);
	}


	CommandBufferFactory::recordCommandBuffer(
	    framesData[currentFrameComp->currentFrame].commandBuffer, imageIndex, textureInfo, swapChain, pipelineHandler,
	    currentFrameComp->currentFrame, bufferManager, meshInfos, *mainCamera, *lightTexture, *materialDSetComponent,
	    *dManager, globalDSetComponent, objectDSetComponent);
}