#include "RenderPasses.hpp"
static void drawMainPass(vk::raii::CommandBuffer& cmd, SwapChain& swapChain, uint32_t currentFrame,
                         BindlessTextureDSetComponent& bindlessTextureDSetComponent,
                         DescriptorManagerComponent& dManager, GlobalDSetComponent& globalDSetComponent,
                         BufferManager& bManager, ModelDSetComponent& objectDSetComponent, ModelManager& mManager,
                         const DrawInfoComponent& drawInfo, PipelineManager& pManager, bool hasSkybox)
{
	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
	                                static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));

	const std::string opaquePipeline = hasSkybox ? "standard_forward_opaque" : "standard_forward_opaque_no_ibl";
	const std::string alphaPipeline = hasSkybox ? "standard_forward_alpha" : "standard_forward_alpha_no_ibl";

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines[opaquePipeline].layout, 0,
	                       dManager.descriptorManager->getSet(globalDSetComponent.globalDSets, currentFrame),
	                       nullptr);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *pManager.pipelines[opaquePipeline].layout, 1,
	    dManager.descriptorManager->getSet(objectDSetComponent.modelBufferDSet, currentFrame), nullptr);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *pManager.pipelines[opaquePipeline].layout, 2,
	    dManager.descriptorManager->getSet(bindlessTextureDSetComponent.bindlessTextureSet), nullptr);

	const uint32_t commandStride = sizeof(VkDrawIndexedIndirectCommand);
	cmd.bindVertexBuffers(0, mManager.vertexIndexBuffers[mManager.meshes[0].vertexIndexBufferID].vertexBuffer, {0});
	cmd.bindIndexBuffer(mManager.vertexIndexBuffers[mManager.meshes[0].vertexIndexBufferID].indexBuffer, 0,
	                    vk::IndexType::eUint32);

	uint32_t opaqueSingleCount = drawInfo.opaqueSingleCount;
	uint32_t opaqueDoubleCount = drawInfo.opaqueDoubleCount;
	uint32_t alphaSingleCount = drawInfo.alphaSingleCount;
	uint32_t alphaDoubleCount = drawInfo.alphaDoubleCount;

	uint32_t currentCommandOffset = 0;
	uint32_t currentCountBufferOffset = 0;

	// Opaque passes
	if (opaqueSingleCount > 0 || opaqueDoubleCount > 0)
	{
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines[opaquePipeline].pipeline);

		// 1. Opaque Single-Sided
		if (opaqueSingleCount > 0)
		{
			cmd.setCullMode(vk::CullModeFlagBits::eBack);
			cmd.drawIndexedIndirectCount(
			    bManager.buffers[objectDSetComponent.compactedDrawBuffer.id].buffer[currentFrame], currentCommandOffset,
			    bManager.buffers[objectDSetComponent.drawCountBuffer.id].buffer[currentFrame], currentCountBufferOffset,
			    opaqueSingleCount, commandStride);
			currentCommandOffset += opaqueSingleCount * commandStride;
		}
		currentCountBufferOffset += sizeof(uint32_t);

		// 2. Opaque Double-Sided
		if (opaqueDoubleCount > 0)
		{
			cmd.setCullMode(vk::CullModeFlagBits::eNone);
			cmd.drawIndexedIndirectCount(
			    bManager.buffers[objectDSetComponent.compactedDrawBuffer.id].buffer[currentFrame], currentCommandOffset,
			    bManager.buffers[objectDSetComponent.drawCountBuffer.id].buffer[currentFrame], currentCountBufferOffset,
			    opaqueDoubleCount, commandStride);
			currentCommandOffset += opaqueDoubleCount * commandStride;
		}
		currentCountBufferOffset += sizeof(uint32_t);
	}
	else
	{
		// Advance offsets if opaque passes are entirely skipped but we need to reach alpha passes
		currentCommandOffset += (opaqueSingleCount + opaqueDoubleCount) * commandStride;
		currentCountBufferOffset += 2 * sizeof(uint32_t);
	}

	// Skybox pass
	if (hasSkybox)
	{
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["skybox"].pipeline);
		cmd.setCullMode(vk::CullModeFlagBits::eNone);
		cmd.draw(3, 1, 0, 0);
	}

	// Alpha test passes
	if (alphaSingleCount > 0 || alphaDoubleCount > 0)
	{
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines[alphaPipeline].pipeline);

		// 3. Alpha Single-Sided
		if (alphaSingleCount > 0)
		{
			cmd.setCullMode(vk::CullModeFlagBits::eBack);
			cmd.drawIndexedIndirectCount(
			    bManager.buffers[objectDSetComponent.compactedDrawBuffer.id].buffer[currentFrame], currentCommandOffset,
			    bManager.buffers[objectDSetComponent.drawCountBuffer.id].buffer[currentFrame], currentCountBufferOffset,
			    alphaSingleCount, commandStride);
			currentCommandOffset += alphaSingleCount * commandStride;
		}
		currentCountBufferOffset += sizeof(uint32_t);

		// 4. Alpha Double-Sided
		if (alphaDoubleCount > 0)
		{
			cmd.setCullMode(vk::CullModeFlagBits::eNone);
			cmd.drawIndexedIndirectCount(
			    bManager.buffers[objectDSetComponent.compactedDrawBuffer.id].buffer[currentFrame], currentCommandOffset,
			    bManager.buffers[objectDSetComponent.drawCountBuffer.id].buffer[currentFrame], currentCountBufferOffset,
			    alphaDoubleCount, commandStride);
		}
	}
}

void RenderPasses::MainPass(SwapChain& swapChain, uint32_t currentFrame,
                            BindlessTextureDSetComponent& bindlessTextureDSetComponent,
                            DescriptorManagerComponent& dManager, GlobalDSetComponent& globalDSetComponent,
                            BufferManager& bManager, ModelDSetComponent& objectDSetComponent, ModelManager& mManager,
                            const DrawInfoComponent& drawInfo, PipelineManager& pManager, bool hasSkybox,
                            GraphicsSettingsComponent& graphicsSettings, RenderGraph& rg)
{
	vk::ClearValue clearBlack = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f);
	vk::ClearValue clearSky = vk::ClearColorValue(0.0f, 0.637f, 1.0f, 1.0f);
	vk::ClearValue clearDepth0 = vk::ClearDepthStencilValue(0.0f, 0);
	if (graphicsSettings.msaaSamples & vk::SampleCountFlagBits::e1) // What a mess
	{
		std::vector<RGResourceAccess> mainWrites = {{"MainColor", RGResourceUsage::ColorAttachmentWrite},
		                                            {"ViewNormals", RGResourceUsage::ColorAttachmentWrite},
		                                            {"Depth", RGResourceUsage::DepthAttachmentWrite}};
		rg.addPass(
		    "Main",
		    {.colorAttachments = {{"MainColor", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clearSky},
		                          {"ViewNormals", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
		                           clearBlack}},
		     .depthAttachment =
		         RGAttachmentConfig{"Depth", vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore, clearDepth0}},
		    {{"shadowMap", RGResourceUsage::ShaderRead}}, std::move(mainWrites),
		    [&, currentFrame, hasSkybox](vk::raii::CommandBuffer& cmd)
		    {
			    drawMainPass(cmd, swapChain, currentFrame, bindlessTextureDSetComponent, dManager,
			                                       globalDSetComponent, bManager, objectDSetComponent, mManager,
			                                       drawInfo, pManager, hasSkybox);
		    });
	}
	else
	{
		std::vector<RGResourceAccess> mainWrites = {{"MainColorMSAA", RGResourceUsage::ColorAttachmentWrite},
		                                            {"ViewNormalsMSAA", RGResourceUsage::ColorAttachmentWrite},
		                                            {"DepthMSAA", RGResourceUsage::DepthAttachmentWrite},
		                                            {"MainColor", RGResourceUsage::ColorAttachmentWrite},
		                                            {"ViewNormals", RGResourceUsage::ColorAttachmentWrite},
		                                            {"Depth", RGResourceUsage::DepthAttachmentWrite}};

		vk::ResolveModeFlagBits colorResolve = vk::ResolveModeFlagBits::eAverage;
		vk::ResolveModeFlagBits depthResolve = vk::ResolveModeFlagBits::eSampleZero;
		rg.addPass(
		    "Main",
		    {.colorAttachments = {{"MainColorMSAA", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clearSky,
		                           "MainColor", colorResolve},
		                          {"ViewNormalsMSAA", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
		                           clearBlack, "ViewNormals", colorResolve}},
		     .depthAttachment = RGAttachmentConfig{"DepthMSAA", vk::AttachmentLoadOp::eLoad,
		                                           vk::AttachmentStoreOp::eStore, clearDepth0, "Depth", depthResolve}},
		    {{"shadowMap", RGResourceUsage::ShaderRead}}, std::move(mainWrites),
		    [&, currentFrame, hasSkybox](vk::raii::CommandBuffer& cmd)
		    {
			    drawMainPass(cmd, swapChain, currentFrame, bindlessTextureDSetComponent, dManager,
			                                       globalDSetComponent, bManager, objectDSetComponent, mManager,
			                                       drawInfo, pManager, hasSkybox);
		    });
	}
}