#include "DirectLightPass.hpp"
#include "PassCommands.hpp"

#include <Orhescyon/GeneralManager.hpp>

#include "../GraphicsContexts.hpp"
#include "../Components/BufferManagerComponent.hpp"
#include "../Components/ModelManagerComponent.hpp"
#include "../Components/TextureManagerComponent.hpp"
#include "../Components/DescriptorManagerComponent.hpp"
#include "../Components/PipelineManagerComponent.hpp"
#include "../Components/DirectLightComponent.hpp"
#include "../Components/DrawInfoComponent.hpp"
#include "../Resources/Components/GlobalDSetComponent.hpp"
#include "../Resources/Components/ModelDSetComponent.hpp"
#include "../Resources/Managers/BufferManager.hpp"
#include "../Resources/Managers/ModelManager.hpp"
#include "../Resources/Managers/TextureManager.hpp"
#include "../Managers/PipelineManager.hpp"
#include "../RenderGraph/RenderGraph.hpp"

void DirectLightPass::addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame)
{
	auto& dManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	auto& globalDSetComponent = *gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	auto& bManager = *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	auto& objectDSetComponent = *gm.getContextComponent<MainDSetsContext, ModelDSetComponent>();
	auto& mManager = *gm.getContextComponent<ModelManagerContext, ModelManagerComponent>()->modelManager;
	auto& drawInfo = *gm.getContextComponent<CurrentFrameContext, DrawInfoComponent>();
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& textureManager = *gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	auto& lightTexture = *gm.getContextComponent<SunContext, DirectLightComponent>();

	vk::ClearValue clearDepth0 = vk::ClearDepthStencilValue(0.0f, 0);
	rg.addPass("ShadowCull", {.isCompute = true}, {}, {},
	           [&, frame](vk::raii::CommandBuffer& cmd)
	           {
		           drawShadowCullPass(cmd, frame, dManager, globalDSetComponent, objectDSetComponent, mManager,
		                              bManager, drawInfo, pManager);
	           });

	rg.addPass("Shadow",
	           {.depthAttachment = RGAttachmentConfig{"shadowMap", vk::AttachmentLoadOp::eClear,
	                                                  vk::AttachmentStoreOp::eStore, clearDepth0},
	            .customExtent =
	                vk::Extent2D{static_cast<uint32_t>(lightTexture.sizeX), static_cast<uint32_t>(lightTexture.sizeY)}},
	           {}, {{"shadowMap", RGResourceUsage::DepthAttachmentWrite}},
	           [&, frame](vk::raii::CommandBuffer& cmd)
	           {
		           drawShadowPass(cmd, frame, lightTexture, dManager, globalDSetComponent, objectDSetComponent,
		                          textureManager, mManager, bManager, drawInfo, pManager);
	           });
}
