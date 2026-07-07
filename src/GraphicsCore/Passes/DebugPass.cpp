#include "DebugPass.hpp"

#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Entitys/EntityManager.hpp>

#include "GraphicsCore/GraphicsContexts.hpp"
#include "GraphicsCore/SwapChain.hpp"
#include "GraphicsCore/Components/SwapChainComponent.hpp"
#include "GraphicsCore/Components/TextureManagerComponent.hpp"
#include "GraphicsCore/Components/DescriptorManagerComponent.hpp"
#include "GraphicsCore/Components/PipelineManagerComponent.hpp"
#include "GraphicsCore/Components/GraphicsSettingsComponent.hpp"
#include "GraphicsCore/Components/ModelManagerComponent.hpp"
#include "GraphicsCore/Components/LightProbeGridComponent.hpp"
#include "GraphicsCore/Components/RelationshipComponent.hpp"
#include "GraphicsCore/Components/GlobalTransformComponent.hpp"
#include "GraphicsCore/Resources/Components/GlobalDSetComponent.hpp"
#include "GraphicsCore/Resources/Components/MeshInfoComponent.hpp"
#include "GraphicsCore/Components/ReflectionProbeComponent.hpp"
#include "GraphicsCore/Resources/Managers/ModelManager.hpp"
#include "GraphicsCore/Resources/Managers/TextureManager.hpp"
#include "GraphicsCore/Managers/PipelineManager.hpp"
#include "GraphicsCore/Factories/PipelineFactory.hpp"
#include "GraphicsCore/RenderGraph/RenderGraph.hpp"

struct AABBPush
{
	glm::mat4 model;
	glm::vec3 aabbMin;
	float p0 = 0.0f;
	glm::vec3 aabbMax;
	float p1 = 0.0f;
};

void DebugPass::onInit(Orhescyon::GeneralManager& gm)
{
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& tManager = *gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	auto depthFormat = tManager.findBestFormat();

	pManager.build(
	    PipelineDescription{
	        .shaderPath = "aabb_debug.spv",
	        .topology = vk::PrimitiveTopology::eLineList,
	        .cullMode = vk::CullModeFlagBits::eNone,
	        .depthTest = true,
	        .depthWrite = false,
	        .depthOp = vk::CompareOp::eGreater,
	        .colorAttachments = {PipelineFactory::opaqueAttachment()},
	        .colorFormats = {swapChain.hdrFormat},
	        .depthFormat = depthFormat,
	        .setLayoutNames = {"globalSet"},
	        .pushConstants = {{vk::ShaderStageFlagBits::eVertex, 0, 96u}},
	    },
	    "aabb_debug");

	pManager.build(
	    PipelineDescription{
	        .shaderPath = "aabb_debug.spv",
	        .topology = vk::PrimitiveTopology::eLineList,
	        .cullMode = vk::CullModeFlagBits::eNone,
	        .depthTest = false,
	        .depthWrite = false,
	        .colorAttachments = {PipelineFactory::opaqueAttachment()},
	        .colorFormats = {swapChain.hdrFormat},
	        .depthFormat = depthFormat,
	        .setLayoutNames = {"globalSet"},
	        .pushConstants = {{vk::ShaderStageFlagBits::eVertex, 0, 96u}},
	    },
	    "aabb_debug_ontop");

	pManager.build(
	    PipelineDescription{
	        .shaderPath = "gi_probe_debug.spv",
	        .topology = vk::PrimitiveTopology::eTriangleList,
	        .cullMode = vk::CullModeFlagBits::eBack,
	        .depthTest = true,
	        .depthWrite = false,
	        .depthOp = vk::CompareOp::eGreater,
	        .colorAttachments = {PipelineFactory::opaqueAttachment()},
	        .colorFormats = {swapChain.hdrFormat},
	        .depthFormat = depthFormat,
	        .setLayoutNames = {"globalSet"},
	        .pushConstants = {{vk::ShaderStageFlagBits::eVertex, 0, 32u}},
	    },
	    "gi_probe_debug");
}

void DebugPass::addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame)
{
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	auto& dManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	auto& globalDSetComponent = *gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& graphicsSettings = *gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>();
	auto& mManager = *gm.getContextComponent<ModelManagerContext, ModelManagerComponent>()->modelManager;

	std::function<void(Orhescyon::Entity, std::vector<AABBPush>&)> draw =
	    [&](Orhescyon::Entity e, std::vector<AABBPush>& pushData)
	{
		if (!gm.isActive(e)) return;
		if (auto* meshComponent = gm.getComponent<MeshInfoComponent>(e))
			if (auto* globalTransform = gm.getComponent<GlobalTransformComponent>(e))
			{
				AABBPush pd;
				pd.model = globalTransform->getGlobalModelMatrix();
				for (const auto& primitive : mManager.meshes[meshComponent->mesh].primitives)
				{
					pd.aabbMin = primitive.AABBMin;
					pd.aabbMax = primitive.AABBMax;
				}
				pushData.push_back(pd);
			}
		if (auto* rel = gm.getComponent<RelationshipComponent>(e))
		{
			Orhescyon::Entity child = rel->firstChild;
			while (child != NULL_ENTITY)
			{
				draw(child, pushData);
				auto* childRel = gm.getComponent<RelationshipComponent>(child);
				child = childRel ? childRel->nextSibling : NULL_ENTITY;
			}
		}
	};
	std::vector<AABBPush> pushData;
	draw(graphicsSettings.selectedEntity, pushData);

	// Reflection probe debug: only for the selected probe, so it toggles with selection.
	// Read from the component (not the GPU buffer) so it also shows before the first bake.
	if (gm.isActive(graphicsSettings.selectedEntity))
	if (auto* refl = gm.getComponent<ReflectionProbeComponent>(graphicsSettings.selectedEntity))
	{
		AABBPush box;
		box.model = glm::mat4(1.0f);
		box.aabbMin = refl->origin - refl->halfExtent;
		box.aabbMax = refl->origin + refl->halfExtent;
		pushData.push_back(box);

		// Small cube marking the capture center.
		const glm::vec3 markerHalf(0.15f);
		AABBPush center;
		center.model = glm::mat4(1.0f);
		center.aabbMin = refl->origin - markerHalf;
		center.aabbMax = refl->origin + markerHalf;
		pushData.push_back(center);
	}

	vk::ClearValue clearDepth0 = vk::ClearDepthStencilValue(0.0f, 0);
	rg.addPass(
	    "AABBDebug",
	    {.colorAttachments = {{"MainColor", vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore}},
	     .depthAttachment =
	         RGAttachmentConfig{"Depth", vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore, clearDepth0}},
	    {}, {{"MainColor", RGResourceUsage::ColorAttachmentWrite}, {"Depth", RGResourceUsage::DepthAttachmentWrite}},
	    [&, pushData, frame](vk::raii::CommandBuffer& cmd)
	    {
		    auto& pip = pManager.pipelines[graphicsSettings.aabbAlwaysOnTop ? "aabb_debug_ontop" : "aabb_debug"];
		    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pip.pipeline);
		    cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
		                                    static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
		    cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));
		    cmd.setCullMode(vk::CullModeFlagBits::eNone);
		    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pip.layout, 0,
		                           dManager.descriptorManager->getSet(globalDSetComponent.globalDSets, frame), nullptr);
		    for (int i = 0; i < pushData.size(); ++i)
		    {
			    cmd.pushConstants<AABBPush>(*pip.layout, vk::ShaderStageFlagBits::eVertex, 0, pushData[i]);
			    cmd.draw(24, 1, 0, 0);
		    }
	    });

	auto* grid = gm.getContextComponent<LightProbeGridContext, LightProbeGridComponent>();
	if (grid != nullptr && grid->debugVisualize)
	{
		struct GIProbePush
		{
			glm::vec3 origin;
			float scale;
			int32_t countX;
			int32_t countY;
			int32_t countZ;
			float spacing;
		};
		GIProbePush push{
		    grid->origin, grid->debugScale, grid->count.x, grid->count.y, grid->count.z, grid->spacing,
		};
		uint32_t probeCount = static_cast<uint32_t>(grid->count.x * grid->count.y * grid->count.z);

		rg.addPass(
		    "GIProbeDebug",
		    {.colorAttachments = {{"MainColor", vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore}},
		     .depthAttachment = RGAttachmentConfig{"Depth", vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore}},
		    {}, {{"MainColor", RGResourceUsage::ColorAttachmentWrite}, {"Depth", RGResourceUsage::DepthAttachmentWrite}},
		    [&, push, probeCount, frame](vk::raii::CommandBuffer& cmd)
		    {
			    auto& pip = pManager.pipelines["gi_probe_debug"];
			    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pip.pipeline);
			    cmd.setCullMode(vk::CullModeFlagBits::eBack);
			    cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
			                                    static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
			    cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));
			    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pip.layout, 0,
			                           dManager.descriptorManager->getSet(globalDSetComponent.globalDSets, frame),
			                           nullptr);
			    cmd.pushConstants<GIProbePush>(*pip.layout, vk::ShaderStageFlagBits::eVertex, 0, push);
			    cmd.draw(384u, probeCount, 0u, 0u);
		    });
	}
}
