#include "ParticleSystemRenderPass.hpp"

#include <Orhescyon/GeneralManager.hpp>

#include "GraphicsCore/GraphicsContexts.hpp"
#include "GraphicsCore/Components/BufferManagerComponent.hpp"
#include "GraphicsCore/Components/ModelManagerComponent.hpp"
#include "GraphicsCore/Components/DescriptorManagerComponent.hpp"
#include "GraphicsCore/Components/PipelineManagerComponent.hpp"
#include "GraphicsCore/Components/DrawInfoComponent.hpp"
#include "GraphicsCore/Resources/Components/GlobalDSetComponent.hpp"
#include "GraphicsCore/Resources/Components/ModelDSetComponent.hpp"
#include "GraphicsCore/Resources/Managers/ModelManager.hpp"
#include "GraphicsCore/Managers/PipelineManager.hpp"
#include "GraphicsCore/Factories/PipelineFactory.hpp"
#include "GraphicsCore/RenderGraph/RenderGraph.hpp"
#include "GraphicsCore/SwapChain.hpp"
#include "GraphicsCore/Components/ExposureBufferComponent.hpp"
#include "GraphicsCore/Components/GraphicsSettingsComponent.hpp"
#include "GraphicsCore/Components/DeltaTimeComponent.hpp"
#include "GraphicsCore/Components/VulkanDeviceComponent.hpp"
#include "GraphicsCore/VulkanUtils.hpp"
#include "GraphicsCore/Components/CurrentFrameComponent.hpp"
#include "GraphicsCore/Components/SwapChainComponent.hpp"
#include "GraphicsCore/Components/RenderGraphComponent.hpp"
#include "GraphicsCore/Components/ParticlesBufferComponent.hpp"
#include "GraphicsCore/Components/TextureManagerComponent.hpp"

const int MAX_NUMBER_PARTICLES = 100000;

struct ParticleMetada
{
	glm::vec3 directionalVector;
	float numberOfParticles;
	uint32_t bottomOfStack;
	uint32_t maxNumberOfPatricles;
	float spawnRadiusMax;
	float spawnRadiusMin;
	float timeToLiveMax;
	float timeToLiveMin;
	float velocityMax;
	float velocityMin;
};

struct drawIndirect
{
	uint32_t vertexCount;
	uint32_t instanceCount;
	uint32_t firstVertex;
	uint32_t firstInstance;
};

void ParticleSystemRenderPass::drawParticlRender(vk::raii::CommandBuffer& cmd, uint32_t frame,
                                                 DescriptorManagerComponent& dManager, BufferManager& bManager,
                                                 PipelineManager& pManager, GlobalDSetComponent& globalDSetComponent,
                                                 BufferHandle& indirectBuffer, uint32_t totalFrames)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["system_render"].pipeline);

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["system_render"].layout, 0,
	                       dManager.descriptorManager->getSet(_dSetParticles, frame), nullptr);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["system_render"].layout, 1,
	                       dManager.descriptorManager->getSet(globalDSetComponent.globalDSets, frame), nullptr);

	cmd.pushConstants<uint32_t>(*pManager.pipelines["system_render"].layout, vk::ShaderStageFlagBits::eVertex, 0,
	                            totalFrames);

	cmd.drawIndirect(bManager.buffers[indirectBuffer.id].buffer[frame], 0, 1, sizeof(drawIndirect));
}

void ParticleSystemRenderPass::onInit(Orhescyon::GeneralManager& gm)
{
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& bManager = *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	auto& dManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	auto& rg = *gm.getContextComponent<RenderGraphContext, RenderGraphComponent>()->renderGraph;
	auto& particlesBuffer = *gm.getContextComponent<ParticlesBufferContext, ParticlesBufferComponent>();
	auto& tManager = *gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	auto depthFormat = tManager.findBestFormat();

	_dSetParticles = dManager.allocate("particleSystemSet", MAX_FRAMES_IN_FLIGHT);

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		dManager.update(_dSetParticles, 0, i, vk::DescriptorType::eStorageBuffer,
		                bManager.buffers[particlesBuffer.particlesBuffer.id].buffer[0]);
		dManager.update(_dSetParticles, 5, i, vk::DescriptorType::eStorageBuffer,
		                bManager.buffers[particlesBuffer.aliveIndicesBufferA.id].buffer[0]);
		dManager.update(_dSetParticles, 6, i, vk::DescriptorType::eStorageBuffer,
		                bManager.buffers[particlesBuffer.aliveIndicesBufferB.id].buffer[0]);
	}

	pManager.build(PipelineDescription{
	    .shaderPath = HALCYON_SHADER_OUT_DIR "/system_render.spv",
	    .cullMode = vk::CullModeFlagBits::eNone,
	    .depthTest = true,
	    .depthWrite = false,
	    .depthOp = vk::CompareOp::eGreaterOrEqual,
	    .colorAttachments = {PipelineFactory::blendedAttachment()},
	    .colorFormats = {swapChain.hdrFormat},
	    .depthFormat = depthFormat,
	    .setLayoutNames = {"particleSystemSet", "globalSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eVertex, 0, sizeof(uint32_t)}},
	});
}

void ParticleSystemRenderPass::addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame)
{
	auto& dManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	auto& bManager = *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	uint32_t totalFrames = gm.getContextComponent<CurrentFrameContext, CurrentFrameComponent>()->frameNumber;
	float deltaTime = gm.getContextComponent<DeltaTimeContext, DeltaTimeComponent>()->deltaTime;
	auto& globalDSetComponent = *gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	auto& indirectBuffer = gm.getContextComponent<ParticlesBufferContext, ParticlesBufferComponent>()->indirectBuffer;

	vk::ClearValue clearSky = vk::ClearColorValue(0.0f, 0.637f, 1.0f, 1.0f);
	vk::ClearValue clearDepth0 = vk::ClearDepthStencilValue(0.0f, 0);

	rg.addPass(
	    "ParticleSystemrender",
	    {.colorAttachments = {{"MainColor", vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore, clearSky}},
	     .depthAttachment = {{"Depth", vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore, clearDepth0}}},
	    {}, {{"MainColor", RGResourceUsage::ColorAttachmentWrite}, {"Depth", RGResourceUsage::DepthAttachmentWrite}},
	    [&, frame, totalFrames, deltaTime](vk::raii::CommandBuffer& cmd)
	    {
		    drawParticlRender(cmd, frame, dManager, bManager, pManager, globalDSetComponent, indirectBuffer, totalFrames);
	    });
}
