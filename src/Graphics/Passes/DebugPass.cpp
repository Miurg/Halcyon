#include "RenderPasses.hpp"
#include "../Components/LightProbeGridComponent.hpp"

struct AABBPush
{
	glm::mat4 model;
	glm::vec3 aabbMin;
	float p0 = 0.0f;
	glm::vec3 aabbMax;
	float p1 = 0.0f;
};

void RenderPasses::DebugPass(SwapChain& swapChain, DescriptorManagerComponent& dManager,
                             GlobalDSetComponent& globalDSetComponent, PipelineManager& pManager, uint32_t currentFrame,
                             RenderGraph& rg, GeneralManager& gm, GraphicsSettingsComponent& graphicsSettings,
                             ModelManager& mManager)
{
	std::function<void(Orhescyon::Entity, std::vector<AABBPush>&)> draw = [&](Orhescyon::Entity e, std::vector<AABBPush>& pushData)
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

	vk::ClearValue clearDepth0 = vk::ClearDepthStencilValue(0.0f, 0);
	rg.addPass(
	    "AABBDebug",
	    {.colorAttachments = {{"MainColor", vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore}},
	     .depthAttachment =
	         RGAttachmentConfig{"Depth", vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore, clearDepth0}},
	    {}, {{"MainColor", RGResourceUsage::ColorAttachmentWrite}, {"Depth", RGResourceUsage::DepthAttachmentWrite}},
	    [&, pushData, currentFrame](vk::raii::CommandBuffer& cmd)
	    {
		    auto& pip = pManager.pipelines[graphicsSettings.aabbAlwaysOnTop ? "aabb_debug_ontop" : "aabb_debug"];
		    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pip.pipeline);
		    cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
		                                    static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
		    cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));
		    cmd.setCullMode(vk::CullModeFlagBits::eNone);
		    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pip.layout, 0,
		                           dManager.descriptorManager->getSet(globalDSetComponent.globalDSets, currentFrame),
		                           nullptr);
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
		    [&, push, probeCount, currentFrame](vk::raii::CommandBuffer& cmd)
		    {
			    auto& pip = pManager.pipelines["gi_probe_debug"];
			    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pip.pipeline);
			    cmd.setCullMode(vk::CullModeFlagBits::eBack);
			    cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
			                                    static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
			    cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));
			    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pip.layout, 0,
			                           dManager.descriptorManager->getSet(globalDSetComponent.globalDSets, currentFrame),
			                           nullptr);
			    cmd.pushConstants<GIProbePush>(*pip.layout, vk::ShaderStageFlagBits::eVertex, 0, push);
			    cmd.draw(384u, probeCount, 0u, 0u);
		    });
	}
}