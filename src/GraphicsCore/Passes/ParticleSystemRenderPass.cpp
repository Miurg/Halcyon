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
                                                 DescriptorManagerComponent& descriptorManager, BufferManager& bufferManager,
                                                 PipelineManager& pipelineManager, GlobalDSetComponent& globalDSetComponent,
                                                 BufferHandle& indirectBuffer, uint32_t totalFrames)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineManager.pipelines["system_render"].pipeline);

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineManager.pipelines["system_render"].layout, 0,
	                       descriptorManager.descriptorManager->getSet(_dSetParticles, frame), nullptr);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineManager.pipelines["system_render"].layout, 1,
	                       descriptorManager.descriptorManager->getSet(globalDSetComponent.globalDSets, frame), nullptr);

	cmd.pushConstants<uint32_t>(*pipelineManager.pipelines["system_render"].layout, vk::ShaderStageFlagBits::eVertex, 0,
	                            totalFrames);

	cmd.drawIndirect(bufferManager.getBuffer(indirectBuffer, frame), 0, 1, sizeof(drawIndirect));
}

void ParticleSystemRenderPass::onInit(Orhescyon::GeneralManager& gm)
{
	auto& pipelineManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& bufferManager = *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	auto& descriptorManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	auto& rg = *gm.getContextComponent<RenderGraphContext, RenderGraphComponent>()->renderGraph;
	auto& particlesBuffer = *gm.getContextComponent<ParticlesBufferContext, ParticlesBufferComponent>();
	auto& textureManager = *gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	auto depthFormat = textureManager.findBestFormat();

	_dSetParticles = descriptorManager.allocate("particleSystemSet", MAX_FRAMES_IN_FLIGHT);

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		descriptorManager.update(_dSetParticles, 0, i, vk::DescriptorType::eStorageBuffer,
		                bufferManager.getBuffer(particlesBuffer.particlesBuffer));
		descriptorManager.update(_dSetParticles, 5, i, vk::DescriptorType::eStorageBuffer,
		                bufferManager.getBuffer(particlesBuffer.aliveIndicesBufferA));
		descriptorManager.update(_dSetParticles, 6, i, vk::DescriptorType::eStorageBuffer,
		                bufferManager.getBuffer(particlesBuffer.aliveIndicesBufferB));
	}

	pipelineManager.build(PipelineDescription{
	    .shaderPath = "system_render.spv",
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
	auto& descriptorManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	auto& bufferManager = *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	auto& pipelineManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
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
		    drawParticlRender(cmd, frame, descriptorManager, bufferManager, pipelineManager, globalDSetComponent, indirectBuffer, totalFrames);
	    });
}
