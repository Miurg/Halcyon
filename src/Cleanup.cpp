#include "Cleanup.hpp"

#include "Graphics/GraphicsContexts.hpp"
#include "Graphics/Components/BufferManagerComponent.hpp"
#include "Graphics/Components/DescriptorManagerComponent.hpp"
#include "Graphics/Components/TextureManagerComponent.hpp"
#include "Graphics/Components/ModelManagerComponent.hpp"
#include "Graphics/Components/SwapChainComponent.hpp"
#include "Graphics/Components/VulkanDeviceComponent.hpp"
#include "Graphics/Factories/SwapChainFactory.hpp"
#include "Graphics/Components/PipelineHandlerComponent.hpp"
#include "Graphics/Components/FrameManagerComponent.hpp"
#include "Graphics/Components/VMAllocatorComponent.hpp"
#include "Platform/Components/WindowComponent.hpp"
#include "Platform/PlatformContexts.hpp"


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
	PipelineHandler* pipelineHandler =
	    gm.getContextComponent<MainSignatureContext, PipelineHandlerComponent>()->pipelineHandler;
	VMAllocatorComponent* vmaComp = gm.getContextComponent<VMAllocatorContext, VMAllocatorComponent>();

	vulkanDevice->device.waitIdle();
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
	if (pipelineHandler)
	{
		delete pipelineHandler;
		pipelineHandler = nullptr;
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
