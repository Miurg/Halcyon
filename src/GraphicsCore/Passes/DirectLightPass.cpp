#include "DirectLightPass.hpp"
#include "GraphicsCore/Passes/PassCommands.hpp"

#include <Orhescyon/GeneralManager.hpp>

#include "GraphicsCore/GraphicsContexts.hpp"
#include "GraphicsCore/Components/BufferManagerComponent.hpp"
#include "GraphicsCore/Components/ModelManagerComponent.hpp"
#include "GraphicsCore/Components/TextureManagerComponent.hpp"
#include "GraphicsCore/Components/DescriptorManagerComponent.hpp"
#include "GraphicsCore/Components/PipelineManagerComponent.hpp"
#include "GraphicsCore/Components/RenderGraphComponent.hpp"
#include "GraphicsCore/Components/DirectLightComponent.hpp"
#include "GraphicsCore/Components/DrawInfoComponent.hpp"
#include "GraphicsCore/Resources/Components/GlobalDSetComponent.hpp"
#include "GraphicsCore/Resources/Components/ModelDSetComponent.hpp"
#include "GraphicsCore/Resources/Components/BindlessTextureDSetComponent.hpp"
#include "GraphicsCore/Resources/Managers/BufferManager.hpp"
#include "GraphicsCore/Resources/Managers/ModelManager.hpp"
#include "GraphicsCore/Resources/Managers/TextureManager.hpp"
#include "GraphicsCore/Resources/Managers/Vertex.hpp"
#include "GraphicsCore/Managers/PipelineManager.hpp"
#include "GraphicsCore/Factories/PipelineFactory.hpp"
#include "GraphicsCore/RenderGraph/RenderGraph.hpp"

void DirectLightPass::onInit(Orhescyon::GeneralManager& gm)
{
	auto& pipelineManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& textureManager = *gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	auto& rg = *gm.getContextComponent<RenderGraphContext, RenderGraphComponent>()->renderGraph;
	auto bindingDesc = Vertex::getBindingDescription();
	auto attrDescs = Vertex::getAttributeDescriptions();
	auto depthFormat = textureManager.findBestFormat();
	std::vector<std::string> mainLayouts = {"globalSet", "modelSet", "textureSet"};

	// Shadow pass writes directly into the imported physical shadow map (no transient image needed)
	rg.setTerminalOutput("shadowMap", "shadowMap");

	pipelineManager.build(PipelineDescription{
	    .shaderPath = "shadow.spv",
	    .fragEntry = "", // vertex only
	    .vertexBindings = {bindingDesc},
	    .vertexAttributes = std::vector<vk::VertexInputAttributeDescription>(attrDescs.begin(), attrDescs.end()),
	    .cullMode = vk::CullModeFlagBits::eBack,
	    .depthTest = true,
	    .depthWrite = true,
	    .depthOp = vk::CompareOp::eGreater,
	    .colorFormats = {},
	    .depthFormat = depthFormat,
	    .rasterizationSamples = vk::SampleCountFlagBits::e1,
	    .setLayoutNames = mainLayouts,
	});

	// Alpha-tested shadow caster: samples base color and discards below alphaCutoff.
	pipelineManager.build(
	    PipelineDescription{
	        .shaderPath = "shadow.spv",
	        .specializationValues = {1}, // ALPHA_TEST_ENABLED=1
	        .vertexBindings = {bindingDesc},
	        .vertexAttributes = std::vector<vk::VertexInputAttributeDescription>(attrDescs.begin(), attrDescs.end()),
	        .cullMode = vk::CullModeFlagBits::eBack,
	        .depthTest = true,
	        .depthWrite = true,
	        .depthOp = vk::CompareOp::eGreater,
	        .colorFormats = {},
	        .depthFormat = depthFormat,
	        .rasterizationSamples = vk::SampleCountFlagBits::e1,
	        .setLayoutNames = mainLayouts,
	    },
	    "shadow_alpha");
}

void DirectLightPass::addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame)
{
	auto& descriptorManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	auto& globalDSetComponent = *gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	auto& bufferManager = *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	auto& objectDSetComponent = *gm.getContextComponent<MainDSetsContext, ModelDSetComponent>();
	auto& modelManager = *gm.getContextComponent<ModelManagerContext, ModelManagerComponent>()->modelManager;
	auto& drawInfo = *gm.getContextComponent<CurrentFrameContext, DrawInfoComponent>();
	auto& pipelineManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& textureManager = *gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	auto& lightTexture = *gm.getContextComponent<SunContext, DirectLightComponent>();
	auto& bindlessTextureDSetComponent = *gm.getContextComponent<MainDSetsContext, BindlessTextureDSetComponent>();

	vk::ClearValue clearDepth0 = vk::ClearDepthStencilValue(0.0f, 0);
	rg.addPass("ShadowCull", {.isCompute = true}, {}, {},
	           [&, frame](vk::raii::CommandBuffer& cmd)
	           {
		           drawShadowCullPass(cmd, frame, descriptorManager, globalDSetComponent, objectDSetComponent, modelManager,
		                              bufferManager, drawInfo, pipelineManager);
	           });

	rg.addPass("Shadow",
	           {.depthAttachment = RGAttachmentConfig{"shadowMap", vk::AttachmentLoadOp::eClear,
	                                                  vk::AttachmentStoreOp::eStore, clearDepth0},
	            .customExtent =
	                vk::Extent2D{static_cast<uint32_t>(lightTexture.sizeX), static_cast<uint32_t>(lightTexture.sizeY)}},
	           {}, {{"shadowMap", RGResourceUsage::DepthAttachmentWrite}},
	           [&, frame](vk::raii::CommandBuffer& cmd)
	           {
		           drawShadowPass(cmd, frame, lightTexture, descriptorManager, globalDSetComponent, objectDSetComponent,
		                          bindlessTextureDSetComponent, textureManager, modelManager, bufferManager, drawInfo, pipelineManager);
	           });
}
