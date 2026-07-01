#include "GraphicsCore/Systems/RenderSystem.hpp"

#include <iostream>
#ifdef HALCYON_DEV_TOOLS
#include <imgui.h>
#endif

#include "GraphicsCore/GraphicsContexts.hpp"
#include "GraphicsCore/SwapChain.hpp"
#include "GraphicsCore/VulkanDevice.hpp"
#include "GraphicsCore/Components/SwapChainComponent.hpp"
#include "GraphicsCore/Components/CurrentFrameComponent.hpp"
#include "GraphicsCore/Components/FrameImageComponent.hpp"
#include "GraphicsCore/Components/TextureManagerComponent.hpp"
#include "GraphicsCore/Components/VulkanDeviceComponent.hpp"
#include "GraphicsCore/Components/DirectLightComponent.hpp"
#include "GraphicsCore/Components/RenderGraphComponent.hpp"
#include "GraphicsCore/Components/GraphicsSettingsComponent.hpp"
#include "GraphicsCore/Resources/Managers/TextureManager.hpp"
#include "GraphicsCore/RenderGraph/RenderGraph.hpp"

#include "GraphicsCore/Passes/IPass.hpp"
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
#ifdef HALCYON_DEV_TOOLS
#include "../Passes/ImGuiPass.hpp"
#endif
#include "../Passes/PresentPass.hpp"
#include "../Passes/DepthPyramidPass.hpp"
#include "../Passes/ExposurePass.hpp"
#include "../Passes/ParticleSystemComputePass.hpp"

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif
#include "../Passes/ParticleSystemRenderPass.hpp"


void RenderSystem::onRegistered(GeneralManager& gm)
{
	auto add = [&](std::unique_ptr<IPass> pass)
	{
		_passes.push_back(std::move(pass));
		_passes.back()->onInit(gm);
	};

	add(std::make_unique<ParticleSystemComputePass>());
	add(std::make_unique<DirectLightPass>());
	add(std::make_unique<CullPass>());
	add(std::make_unique<DepthPrepass>());
	add(std::make_unique<DepthPyramidPass>());
	add(std::make_unique<GTAOPass>());
	add(std::make_unique<MainPass>());
	add(std::make_unique<ParticleSystemRenderPass>());
	add(std::make_unique<ExposurePass>());
	add(std::make_unique<DebugPass>());
	add(std::make_unique<BloomPass>());
	add(std::make_unique<ToneMappingPass>());
	add(std::make_unique<FXAAPass>());
	add(std::make_unique<VignettePass>());
#ifdef HALCYON_DEV_TOOLS
	add(std::make_unique<ImGuiPass>());
#endif
	add(std::make_unique<PresentPass>());

	std::cout << "RenderSystem registered!" << std::endl;
}

void RenderSystem::onShutdown(GeneralManager& gm)
{
	for (auto it = _passes.rbegin(); it != _passes.rend(); ++it) (*it)->onShutdown(gm);
	_passes.clear();
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
	rg.importImage("swapChainImage", swapChain.swapChainImages[imageIndex], swapChain.swapChainImageViews[imageIndex],
	               vk::ImageAspectFlagBits::eColor);
}

void RenderSystem::applySettingsChanges(GeneralManager& gm)
{
	auto& graphicsSettings = *gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>();
	const bool msaaChanged = graphicsSettings.msaaSamples != graphicsSettings.appliedMsaaSamples;
	const bool gtaoChanged = graphicsSettings.enableGtao != graphicsSettings.appliedGtao;
	const bool autoExposureChanged = graphicsSettings.enableAutoExposure != graphicsSettings.appliedAutoExposure;
	if (msaaChanged || gtaoChanged || autoExposureChanged)
	{
		auto& vulkanDevice =
		    *gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
		vulkanDevice.device.waitIdle();

		for (auto& pass : _passes) pass->onSettingsChanged(gm);

		if (msaaChanged)
		{
			auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
			auto& rg = *gm.getContextComponent<RenderGraphContext, RenderGraphComponent>()->renderGraph;
			rg.handleResize(swapChain.swapChainExtent.width, swapChain.swapChainExtent.height);
		}

		graphicsSettings.appliedMsaaSamples = graphicsSettings.msaaSamples;
		graphicsSettings.appliedGtao = graphicsSettings.enableGtao;
		graphicsSettings.appliedAutoExposure = graphicsSettings.enableAutoExposure;
	}
}

void RenderSystem::update(GeneralManager& gm)
{
#ifdef TRACY_ENABLE
	ZoneScopedN("RenderSystem");
#endif

	auto& currentFrameComp = *gm.getContextComponent<CurrentFrameContext, CurrentFrameComponent>();
	if (!currentFrameComp.frameValid) return;

#ifdef HALCYON_DEV_TOOLS
	ImGui::Render();
#endif

	auto& rg = *gm.getContextComponent<RenderGraphContext, RenderGraphComponent>()->renderGraph;
	uint32_t frame = currentFrameComp.currentFrame;
	uint32_t imageIndex = gm.getContextComponent<FrameImageContext, FrameImageComponent>()->imageIndex;

	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	const uint32_t curW = swapChain.swapChainExtent.width;
	const uint32_t curH = swapChain.swapChainExtent.height;
	if (curW != _lastWidth || curH != _lastHeight)
	{
		rg.handleResize(curW, curH);
		for (auto& pass : _passes) pass->onResize(gm, curW, curH);
		_lastWidth = curW;
		_lastHeight = curH;
	}

	importFrameResources(gm, rg, imageIndex);
	applySettingsChanges(gm);

	for (auto& pass : _passes)
		if (pass->isEnabled(gm)) pass->addToGraph(gm, rg, frame);
}
