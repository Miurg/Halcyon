#include "ParticleSystemComputePass.hpp"

#include <Orhescyon/GeneralManager.hpp>

#include "../GraphicsContexts.hpp"
#include "../Components/BufferManagerComponent.hpp"
#include "../Components/ModelManagerComponent.hpp"
#include "../Components/DescriptorManagerComponent.hpp"
#include "../Components/PipelineManagerComponent.hpp"
#include "../Components/DrawInfoComponent.hpp"
#include "../Resources/Components/GlobalDSetComponent.hpp"
#include "../Resources/Components/ModelDSetComponent.hpp"
#include "../Resources/Managers/ModelManager.hpp"
#include "../Managers/PipelineManager.hpp"
#include "../Factories/PipelineFactory.hpp"
#include "../RenderGraph/RenderGraph.hpp"
#include "../SwapChain.hpp"
#include "../Components/ExposureBufferComponent.hpp"
#include "../Components/GraphicsSettingsComponent.hpp"
#include "../Components/DeltaTimeComponent.hpp"
#include "../Components/VulkanDeviceComponent.hpp"
#include "../VulkanUtils.hpp"
#include "../Components/ParticlePropertiesComponent.hpp"
#include "../Components/CurrentFrameComponent.hpp"
#include "../Components/VMAllocatorComponent.hpp"
#include "../Components/ParticlesBufferComponent.hpp"

const int MAX_NUMBER_PARTICLES = 100000;

struct ParticleMetada
{
	float numberOfParticles;
	int bottomOfStack;
	int maxNumberOfPatricles;
	float spawnRadiusMax;
	float spawnRadiusMin;
	float timeToLiveMax;
	float timeToLiveMin;
};

struct dispatchIndirect
{
	uint32_t x;
	uint32_t y;
	uint32_t z;
	uint32_t spawnCount;
};

struct drawIndirect
{
	uint32_t vertexCount;
	uint32_t instanceCount;
	uint32_t firstVertex;
	uint32_t firstInstance;
};

void ParticleSystemComputePass::drawParticleCompute(vk::raii::CommandBuffer& cmd, uint32_t frame,
                                                    DescriptorManagerComponent& dManager, BufferManager& bManager,
                                                    PipelineManager& pManager, uint32_t totalFrames, float deltaTime)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pManager.pipelines["particles_spawner"].pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pManager.pipelines["particles_spawner"].layout, 0,
	                       dManager.descriptorManager->getSet(_dSetParticles, frame), nullptr);

	cmd.pushConstants<uint32_t>(*pManager.pipelines["particles_spawner"].layout, vk::ShaderStageFlagBits::eCompute, 0,
	                            totalFrames);

	cmd.dispatchIndirect(bManager.buffers[_dispatchBuffer.id].buffer[frame], 0);

	vk::MemoryBarrier2 fillBarrier;
	fillBarrier.srcStageMask = vk::PipelineStageFlagBits2::eDrawIndirect;
	fillBarrier.srcAccessMask = vk::AccessFlagBits2::eIndirectCommandRead;
	fillBarrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;
	fillBarrier.dstAccessMask = vk::AccessFlagBits2::eTransferWrite;

	vk::DependencyInfo fillDepInfo;
	fillDepInfo.memoryBarrierCount = 1;
	fillDepInfo.pMemoryBarriers = &fillBarrier;
	cmd.pipelineBarrier2(fillDepInfo);

	cmd.fillBuffer(bManager.buffers[_dispatchBuffer.id].buffer[frame], 0, 4,
	               0); // Nulling x
	cmd.fillBuffer(bManager.buffers[_dispatchBuffer.id].buffer[frame], 12, 4,
	               0); // Nulling spawnCount
	cmd.fillBuffer(bManager.buffers[_indirectBuffer.id].buffer[frame], 4, 4, 0);

	vk::MemoryBarrier2 emiterBarrier;
	emiterBarrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
	emiterBarrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
	emiterBarrier.dstStageMask = vk::PipelineStageFlagBits2::eComputeShader;
	emiterBarrier.dstAccessMask = vk::AccessFlagBits2::eShaderWrite | vk::AccessFlagBits2::eShaderRead;

	vk::DependencyInfo emiterDepInfo;
	emiterDepInfo.memoryBarrierCount = 1;
	emiterDepInfo.pMemoryBarriers = &emiterBarrier;
	cmd.pipelineBarrier2(emiterDepInfo);

	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pManager.pipelines["particles_emiter"].pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pManager.pipelines["particles_emiter"].layout, 0,
	                       dManager.descriptorManager->getSet(_dSetParticles, frame), nullptr);

	cmd.pushConstants<float>(*pManager.pipelines["particles_emiter"].layout, vk::ShaderStageFlagBits::eCompute, 0,
	                         deltaTime);

	uint32_t groupCount = (MAX_NUMBER_PARTICLES + 63) / 64;
	cmd.dispatch(groupCount, 1, 1);

	vk::MemoryBarrier2 renderReadyBarrier;
	renderReadyBarrier.srcStageMask = vk::PipelineStageFlagBits2::eComputeShader;
	renderReadyBarrier.srcAccessMask = vk::AccessFlagBits2::eShaderWrite;
	renderReadyBarrier.dstStageMask = vk::PipelineStageFlagBits2::eDrawIndirect;
	renderReadyBarrier.dstAccessMask = vk::AccessFlagBits2::eIndirectCommandRead;

	vk::DependencyInfo renderReadyDepInfo;
	renderReadyDepInfo.memoryBarrierCount = 1;
	renderReadyDepInfo.pMemoryBarriers = &renderReadyBarrier;
	cmd.pipelineBarrier2(renderReadyDepInfo);
}

void ParticleSystemComputePass::onInit(Orhescyon::GeneralManager& gm)
{
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& bManager = *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	auto& dManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
	auto allocator = gm.getContextComponent<VMAllocatorContext, VMAllocatorComponent>()->allocator;

	_dSetParticles = dManager.allocate("particleSystemSet", MAX_FRAMES_IN_FLIGHT);

	// TODO: memory property floags
	_particlesBuffer = bManager.createBuffer(
	    vk::MemoryPropertyFlagBits::eHostVisible, sizeof(ParticlePropertiesComponent) * MAX_NUMBER_PARTICLES, 1,
	    vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);

	_particlesStack =
	    bManager.createBuffer(vk::MemoryPropertyFlagBits::eHostVisible, sizeof(uint32_t) * MAX_NUMBER_PARTICLES, 1,
	                          vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);

	_particlesMetada =
	    bManager.createBuffer(vk::MemoryPropertyFlagBits::eHostVisible, sizeof(ParticleMetada), 1,
	                          vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);

	_dispatchBuffer =
	    bManager.createBuffer(vk::MemoryPropertyFlagBits::eDeviceLocal, sizeof(dispatchIndirect), MAX_FRAMES_IN_FLIGHT,
	                          vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer |
	                              vk::BufferUsageFlagBits::eTransferDst);

	_indirectBuffer =
	    bManager.createBuffer(vk::MemoryPropertyFlagBits::eDeviceLocal, sizeof(drawIndirect), MAX_FRAMES_IN_FLIGHT,
	                          vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer |
	                              vk::BufferUsageFlagBits::eTransferDst);

	_aliveIndicesBuffer =
	    bManager.createBuffer(vk::MemoryPropertyFlagBits::eDeviceLocal, sizeof(uint32_t) * MAX_NUMBER_PARTICLES, 1,
	                          vk::BufferUsageFlagBits::eStorageBuffer);

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		dManager.update(_dSetParticles, 0, i, vk::DescriptorType::eStorageBuffer,
		                bManager.buffers[_particlesBuffer.id].buffer[0]);
		dManager.update(_dSetParticles, 1, i, vk::DescriptorType::eStorageBuffer,
		                bManager.buffers[_particlesStack.id].buffer[0]);
		dManager.update(_dSetParticles, 2, i, vk::DescriptorType::eStorageBuffer,
		                bManager.buffers[_particlesMetada.id].buffer[0]);
		dManager.update(_dSetParticles, 3, i, vk::DescriptorType::eStorageBuffer,
		                bManager.buffers[_dispatchBuffer.id].buffer[i]);
		dManager.update(_dSetParticles, 4, i, vk::DescriptorType::eStorageBuffer,
		                bManager.buffers[_indirectBuffer.id].buffer[i]);
		dManager.update(_dSetParticles, 5, i, vk::DescriptorType::eStorageBuffer,
		                bManager.buffers[_aliveIndicesBuffer.id].buffer[0]);
	}

	auto& vulkanDevice = *gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;

	std::vector<uint32_t> sequenceData(MAX_NUMBER_PARTICLES);
	for (uint32_t i = 0; i < MAX_NUMBER_PARTICLES; ++i)
	{
		sequenceData[i] = i;
	}

	StagingBuffer staging = VulkanUtils::createStagingBuffer(
	    sequenceData.data(), sizeof(uint32_t) * MAX_NUMBER_PARTICLES, allocator, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	auto cmd = VulkanUtils::beginSingleTimeCommands(vulkanDevice);

	// Stack
	vk::BufferCopy copyRegion;
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = sizeof(uint32_t) * MAX_NUMBER_PARTICLES;

	cmd.copyBuffer(vk::Buffer(staging.buffer), bManager.buffers[_particlesStack.id].buffer[0], copyRegion);

	// Metadata
	ParticleMetada initialMetadata = {.numberOfParticles = 0.0f,
	                                  .bottomOfStack = 0,
	                                  .maxNumberOfPatricles = MAX_NUMBER_PARTICLES - 1,
	                                  .spawnRadiusMax = 5.0f,
	                                  .spawnRadiusMin = 0.0f,
	                                  .timeToLiveMax = 3.0f,
	                                  .timeToLiveMin = 1.0f};
	cmd.updateBuffer(bManager.buffers[_particlesMetada.id].buffer[0], 0,
	                 vk::ArrayProxy<const ParticleMetada>(1, &initialMetadata));

	// Indirect
	drawIndirect initialDraw = {.vertexCount = 6, .instanceCount = 0, .firstVertex = 0, .firstInstance = 0};
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		cmd.updateBuffer(bManager.buffers[_indirectBuffer.id].buffer[i], 0,
		                 vk::ArrayProxy<const drawIndirect>(1, &initialDraw));
	}

	// dispatch
	dispatchIndirect initialDispatch = {.x = 100, .y = 1, .z = 1, .spawnCount = 100};
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		cmd.updateBuffer(bManager.buffers[_dispatchBuffer.id].buffer[i], 0,
		                 vk::ArrayProxy<const dispatchIndirect>(1, &initialDispatch));
	}
	VulkanUtils::endSingleTimeCommands(cmd, vulkanDevice);

	VulkanUtils::destroyStagingBuffer(staging, allocator);
	pManager.build(PipelineDescription{
	    .isCompute = true,
	    .shaderPath = "shaders/particles_spawner.spv",
	    .setLayoutNames = {"particleSystemSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t)}},
	});

	pManager.build(PipelineDescription{
	    .isCompute = true,
	    .shaderPath = "shaders/particles_emiter.spv",
	    .setLayoutNames = {"particleSystemSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(float)}},
	});

	Orhescyon::Entity e = gm.createEntity();
	gm.registerContext<ParticlesBufferContext>(e);
	gm.addComponent<ParticlesBufferComponent>(e, _particlesBuffer, _indirectBuffer, _aliveIndicesBuffer);
}

void ParticleSystemComputePass::addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame)
{
	auto& dManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	auto& bManager = *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	uint32_t totalFrames = gm.getContextComponent<CurrentFrameContext, CurrentFrameComponent>()->frameNumber;
	float deltaTime = gm.getContextComponent<DeltaTimeContext, DeltaTimeComponent>()->deltaTime;

	rg.addPass("ParticleSystemCompute", {.isCompute = true}, {}, {},
	           [&, frame, totalFrames, deltaTime](vk::raii::CommandBuffer& cmd)
	           { drawParticleCompute(cmd, frame, dManager, bManager, pManager, totalFrames, deltaTime); });
}
