#include "Cleanup.hpp"

#include "GraphicsCore/GraphicsContexts.hpp"
#include "GraphicsCore/Components/BufferManagerComponent.hpp"
#include "GraphicsCore/Components/DescriptorManagerComponent.hpp"
#include "GraphicsCore/Components/TextureManagerComponent.hpp"
#include "GraphicsCore/Components/ModelManagerComponent.hpp"
#include "GraphicsCore/Components/SwapChainComponent.hpp"
#include "GraphicsCore/Components/VulkanDeviceComponent.hpp"
#include "GraphicsCore/Factories/SwapChainFactory.hpp"
#include "GraphicsCore/Components/FrameManagerComponent.hpp"
#include "GraphicsCore/Components/VMAllocatorComponent.hpp"
#include "GraphicsCore/Components/RenderGraphComponent.hpp"
#include "GraphicsCore/RenderGraph/RenderGraph.hpp"
#include "PlatformCore/PlatformContexts.hpp"
#include "PlatformCore/Components/WindowComponent.hpp"
#include "PlatformCore/Window.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include "GraphicsCore/Managers/PipelineManager.hpp"
#include "GraphicsCore/Components/PipelineManagerComponent.hpp"
#include "PhysicsCore/PhysicsCleanup.hpp"

#ifdef TRACY_ENABLE
#include <tracy/TracyVulkan.hpp>
#endif

void Cleanup::cleanup(GeneralManager& gm)
{
	DescriptorManagerComponent* dManagerComp = gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	BufferManagerComponent* bManagerComp = gm.getContextComponent<BufferManagerContext, BufferManagerComponent>();
	TextureManagerComponent* tManagerComp = gm.getContextComponent<TextureManagerContext, TextureManagerComponent>();
	ModelManagerComponent* mManagerComp = gm.getContextComponent<ModelManagerContext, ModelManagerComponent>();
	FrameManagerComponent* fManagerComp = gm.getContextComponent<FrameManagerContext, FrameManagerComponent>();
	SwapChainComponent* swapComp = gm.getContextComponent<MainSwapChainContext, SwapChainComponent>();
	VulkanDeviceComponent* vulkanDeviceComp = gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>();
	VMAllocatorComponent* vmaComp = gm.getContextComponent<VMAllocatorContext, VMAllocatorComponent>();
	RenderGraphComponent* rgComp = gm.getContextComponent<RenderGraphContext, RenderGraphComponent>();
	PipelineManagerComponent* pManagerComp = gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>();
	WindowComponent* windowComp = gm.getContextComponent<MainWindowContext, WindowComponent>();

	DescriptorManager* dManager = dManagerComp ? dManagerComp->descriptorManager : nullptr;
	BufferManager* bManager = bManagerComp ? bManagerComp->bufferManager : nullptr;
	TextureManager* tManager = tManagerComp ? tManagerComp->textureManager : nullptr;
	ModelManager* mManager = mManagerComp ? mManagerComp->modelManager : nullptr;
	FrameManager* fManager = fManagerComp ? fManagerComp->frameManager : nullptr;
	SwapChain* swap = swapComp ? swapComp->swapChainInstance : nullptr;
	VulkanDevice* vulkanDevice = vulkanDeviceComp ? vulkanDeviceComp->vulkanDeviceInstance : nullptr;
	RenderGraph* rg = rgComp ? rgComp->renderGraph : nullptr;
	PipelineManager* pManager = pManagerComp ? pManagerComp->pipelineManager : nullptr;

	if (vulkanDevice)
	{
		vulkanDevice->device.waitIdle();
	}

#ifdef TRACY_ENABLE
	if (vulkanDevice && vulkanDevice->tracyContext)
	{
		TracyVkDestroy(vulkanDevice->tracyContext);
		vulkanDevice->tracyContext = nullptr;
	}
#endif

	if (ImGui::GetCurrentContext())
	{
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	if (bManager)
	{
		delete bManager;
		bManager = nullptr;
	}
	if (tManager)
	{
		delete tManager;
		tManager = nullptr;
	}
	if (mManager)
	{
		delete mManager;
		mManager = nullptr;
	}
	if (dManager)
	{
		delete dManager;
		dManager = nullptr;
	}
	if (fManager)
	{
		delete fManager;
		fManager = nullptr;
	}
	if (swap)
	{
		SwapChainFactory::cleanupSwapChain(*swap);
		delete swap;
		swap = nullptr;
	}
	if (pManager)
	{
		delete pManager;
		pManager = nullptr;
	}
	if (rg)
	{
		delete rg;
		rg = nullptr;
	}
	if (vmaComp && vmaComp->allocator)
	{
		vmaDestroyAllocator(vmaComp->allocator);
		vmaComp->allocator = nullptr;
	}
	if (vulkanDevice)
	{
		delete vulkanDevice;
		vulkanDevice = nullptr;
	}
	if (windowComp && windowComp->windowInstance)
	{
		delete windowComp->windowInstance;
		windowComp->windowInstance = nullptr;
	}

	PhysicsCleanup::cleanup(gm);
}
