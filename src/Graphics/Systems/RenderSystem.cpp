#include "RenderSystem.hpp"
#include <iostream>
#include "../GraphicsContexts.hpp"
#include <imgui.h>
#include "../Factories/CommandBufferFactory.hpp"
#include "../Components/PipelineHandlerComponent.hpp"
#include "../Components/SwapChainComponent.hpp"
#include "../Components/BufferManagerComponent.hpp"
#include "../FrameData.hpp"
#include "../Components/FrameDataComponent.hpp"
#include "../Components/CurrentFrameComponent.hpp"
#include "../Components/DescriptorManagerComponent.hpp"
#include "../Components/FrameImageComponent.hpp"
#include "../Components/TextureManagerComponent.hpp"
#include "../Resources/Managers/TextureManager.hpp"
#include "../Components/ModelManagerComponent.hpp"
#include "../Managers/FrameManager.hpp"
#include "../Components/FrameManagerComponent.hpp"
#include "../Components/DrawInfoComponent.hpp"
#include "../Components/SsaoSettingsComponent.hpp"

void RenderSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "RenderSystem registered!" << std::endl;
}

void RenderSystem::onShutdown(GeneralManager& gm)
{
	std::cout << "RenderSystem shutdown!" << std::endl;
}

void RenderSystem::update(GeneralManager& gm)
{
	SwapChain& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	PipelineHandler& pipelineHandler =
	    *gm.getContextComponent<MainSignatureContext, PipelineHandlerComponent>()->pipelineHandler;
	BufferManager& bufferManager =
	    *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	TextureManager& textureManager =
	    *gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	ModelManager& modelManager = *gm.getContextComponent<ModelManagerContext, ModelManagerComponent>()->modelManager;
	FrameManager* frameManager = gm.getContextComponent<FrameManagerContext, FrameManagerComponent>()->frameManager;
	CurrentFrameComponent* currentFrameComp = gm.getContextComponent<CurrentFrameContext, CurrentFrameComponent>();
	LightComponent* lightTexture = gm.getContextComponent<SunContext, LightComponent>();
	BindlessTextureDSetComponent* materialDSetComponent =
	    gm.getContextComponent<MainDSetsContext, BindlessTextureDSetComponent>();
	DescriptorManagerComponent* dManager =
	    gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	GlobalDSetComponent* globalDSetComponent = gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	ModelDSetComponent* objectDSetComponent = gm.getContextComponent<MainDSetsContext, ModelDSetComponent>();
	DrawInfoComponent* drawInfo = gm.getContextComponent<CurrentFrameContext, DrawInfoComponent>();
	SsaoSettingsComponent* ssaoSettings = gm.getContextComponent<SsaoSettingsContext, SsaoSettingsComponent>();
	uint32_t imageIndex = gm.getContextComponent<FrameImageContext, FrameImageComponent>()->imageIndex;
	if (!currentFrameComp->frameValid) return;

	ImGui::Render();

	CommandBufferFactory::recordShadowCommandBuffer(
	    frameManager->frames[currentFrameComp->currentFrame].secondaryCommandBuffers[0], pipelineHandler,
	    currentFrameComp->currentFrame, *lightTexture, *dManager, globalDSetComponent, objectDSetComponent,
	    textureManager, modelManager);
	CommandBufferFactory::recordCullCommandBuffer(
	    frameManager->frames[currentFrameComp->currentFrame].secondaryCommandBuffers[1], pipelineHandler,
	    currentFrameComp->currentFrame, *dManager, globalDSetComponent, objectDSetComponent, modelManager, *drawInfo);
	CommandBufferFactory::recordDepthPrepassCommandBuffer(
	    frameManager->frames[currentFrameComp->currentFrame].secondaryCommandBuffers[2], imageIndex, swapChain,
	    pipelineHandler, currentFrameComp->currentFrame, *materialDSetComponent, *dManager, globalDSetComponent,
	    bufferManager, objectDSetComponent, modelManager, textureManager, *drawInfo);
	CommandBufferFactory::recordMainCommandBuffer(
	    frameManager->frames[currentFrameComp->currentFrame].secondaryCommandBuffers[3], imageIndex, swapChain,
	    pipelineHandler, currentFrameComp->currentFrame, *materialDSetComponent, *dManager, globalDSetComponent,
	    bufferManager, objectDSetComponent, modelManager, textureManager, *drawInfo);
	CommandBufferFactory::recordSsaoCommandBuffer(
	    frameManager->frames[currentFrameComp->currentFrame].secondaryCommandBuffers[4], swapChain, pipelineHandler,
	    *dManager, globalDSetComponent->ssaoDSets, globalDSetComponent->globalDSets, textureManager, *ssaoSettings);
	CommandBufferFactory::recordSsaoBlurCommandBuffer(
	    frameManager->frames[currentFrameComp->currentFrame].secondaryCommandBuffers[5], swapChain, pipelineHandler,
	    *dManager, globalDSetComponent->ssaoBlurDSets, textureManager);
	CommandBufferFactory::recordFxaaCommandBuffer(
	    frameManager->frames[currentFrameComp->currentFrame].secondaryCommandBuffers[6], imageIndex, swapChain,
	    pipelineHandler, *dManager, globalDSetComponent->fxaaDSets);
	CommandBufferFactory::executeSecondaryBuffers(
	    frameManager->frames[currentFrameComp->currentFrame].commandBuffer,
	    frameManager->frames[currentFrameComp->currentFrame].secondaryCommandBuffers);
}