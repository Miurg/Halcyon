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

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include "Graphics/Managers/PipelineManager.hpp"
#include "Graphics/Components/PipelineManagerComponent.hpp"

void Cleanup::cleanup(GeneralManager& gm)
{
	DescriptorManager* dManager =
	    gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
	BufferManager* bManager = gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	TextureManager* tManager = gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	ModelManager* mManager = gm.getContextComponent<ModelManagerContext, ModelManagerComponent>()->modelManager;
	FrameManager* fManager = gm.getContextComponent<FrameManagerContext, FrameManagerComponent>()->frameManager;
	SwapChain* swap = gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	VulkanDevice* vulkanDevice =
	    gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	VMAllocatorComponent* vmaComp = gm.getContextComponent<VMAllocatorContext, VMAllocatorComponent>();
	RenderGraph* rg = gm.getContextComponent<RenderGraphContext, RenderGraphComponent>()->renderGraph;
	PipelineManager* pManager =
	    gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;

	vulkanDevice->device.waitIdle();

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

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
	SwapChainFactory::cleanupSwapChain(*swap);
	if (swap)
	{
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
}
