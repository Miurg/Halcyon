#include "RenderSystem.hpp"

#include <iostream>
#include <imgui.h>

#include "../GraphicsContexts.hpp"
#include "../Components/SwapChainComponent.hpp"
#include "../Components/CurrentFrameComponent.hpp"
#include "../Components/FrameImageComponent.hpp"
#include "../Components/TextureManagerComponent.hpp"
#include "../Components/DirectLightComponent.hpp"
#include "../Components/RenderGraphComponent.hpp"
#include "../Components/GraphicsSettingsComponent.hpp"
#include "../Resources/Managers/TextureManager.hpp"
#include "../RenderGraph/RenderGraph.hpp"
#include "../GraphicsInit/GraphicsPipelinesInit.hpp"

#include "../Passes/IPass.hpp"
#include "../Passes/DirectLightPass.hpp"
#include "../Passes/CullPass.hpp"
#include "../Passes/DepthPrepass.hpp"
#include "../Passes/MainPass.hpp"
#include "../Passes/DebugPass.hpp"
#include "../Passes/GTAOPass.hpp"
#include "../Passes/BloomPass.hpp"
#include "../Passes/ToneMappingPass.hpp"
#include "../Passes/FXAAPass.hpp"
#include "../Passes/VignettePass.hpp"
#include "../Passes/ImGuiPass.hpp"
#include "../Passes/PresentPass.hpp"
#include "../Passes/HiZBuildPass.hpp"

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

void RenderSystem::onRegistered(GeneralManager& gm)
{
	m_passes.push_back(std::make_unique<DirectLightPass>());
	m_passes.push_back(std::make_unique<CullPass>());
	m_passes.push_back(std::make_unique<DepthPrepass>());
	m_passes.push_back(std::make_unique<HiZBuildPass>());
	m_passes.push_back(std::make_unique<GTAOPass>());
	m_passes.push_back(std::make_unique<MainPass>());
	m_passes.push_back(std::make_unique<DebugPass>());
	m_passes.push_back(std::make_unique<BloomPass>());
	m_passes.push_back(std::make_unique<ToneMappingPass>());
	m_passes.push_back(std::make_unique<FXAAPass>());
	m_passes.push_back(std::make_unique<VignettePass>());
	m_passes.push_back(std::make_unique<ImGuiPass>());
	m_passes.push_back(std::make_unique<PresentPass>());

	std::cout << "RenderSystem registered!" << std::endl;
}

void RenderSystem::onShutdown(GeneralManager& gm)
{
	m_passes.clear();
	std::cout << "RenderSystem shutdown!" << std::endl;
}

void RenderSystem::importFrameResources(GeneralManager& gm, RenderGraph& rg, uint32_t imageIndex)
{
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	auto& textureManager = *gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	auto& shadowMap = *gm.getContextComponent<SunContext, DirectLightComponent>();

	rg.importImage("shadowMap", textureManager.textures[shadowMap.textureShadowImage.id].textureImage,
	               textureManager.textures[shadowMap.textureShadowImage.id].textureImageView,
	               vk::ImageAspectFlagBits::eDepth);
	rg.importImage("swapChainImage", swapChain.swapChainImages[imageIndex],
	               swapChain.swapChainImageViews[imageIndex], vk::ImageAspectFlagBits::eColor);
	rg.importImage("NoiseImage", textureManager.textures[swapChain.gtaoNoiseTextureHandle.id].textureImage,
	               textureManager.textures[swapChain.gtaoNoiseTextureHandle.id].textureImageView,
	               vk::ImageAspectFlagBits::eColor, vk::ImageLayout::eShaderReadOnlyOptimal);
}

void RenderSystem::applyPendingMsaaChange(GeneralManager& gm)
{
	auto& graphicsSettings = *gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>();
	const bool msaaChanged = graphicsSettings.msaaSamples != graphicsSettings.appliedMsaaSamples;
	const bool gtaoChanged = graphicsSettings.enableGtao != graphicsSettings.appliedGtao;
	if (msaaChanged || gtaoChanged)
	{
		GraphicsPipelinesInit::recreateMsaaPipelines(gm, graphicsSettings.msaaSamples);
		graphicsSettings.appliedMsaaSamples = graphicsSettings.msaaSamples;
		graphicsSettings.appliedGtao = graphicsSettings.enableGtao;
	}
}

void RenderSystem::update(GeneralManager& gm)
{
#ifdef TRACY_ENABLE
	ZoneScopedN("RenderSystem");
#endif

	auto& currentFrameComp = *gm.getContextComponent<CurrentFrameContext, CurrentFrameComponent>();
	if (!currentFrameComp.frameValid) return;

	ImGui::Render();

	auto& rg = *gm.getContextComponent<RenderGraphContext, RenderGraphComponent>()->renderGraph;
	uint32_t frame = currentFrameComp.currentFrame;
	uint32_t imageIndex = gm.getContextComponent<FrameImageContext, FrameImageComponent>()->imageIndex;

	importFrameResources(gm, rg, imageIndex);
	applyPendingMsaaChange(gm);

	for (auto& pass : m_passes)
		if (pass->isEnabled(gm)) pass->addToGraph(gm, rg, frame);
}
