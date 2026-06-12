#include "Cleanup.hpp"

#include "Graphics/GraphicsContexts.hpp"
#include "Graphics/Components/BufferManagerComponent.hpp"
#include "Graphics/Components/DescriptorManagerComponent.hpp"
#include "Graphics/Components/TextureManagerComponent.hpp"
#include "Graphics/Components/ModelManagerComponent.hpp"
#include "Graphics/Components/SwapChainComponent.hpp"
#include "Graphics/Components/VulkanDeviceComponent.hpp"
#include "Graphics/Factories/SwapChainFactory.hpp"
#include "Graphics/Components/FrameManagerComponent.hpp"
#include "Graphics/Components/VMAllocatorComponent.hpp"
#include "Graphics/Components/RenderGraphComponent.hpp"
#include "Graphics/RenderGraph/RenderGraph.hpp"
#include "Platform/PlatformContexts.hpp"
#include "Platform/Components/WindowComponent.hpp"
#include "Platform/Window.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include "Graphics/Managers/PipelineManager.hpp"
#include "Graphics/Components/PipelineManagerComponent.hpp"
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
