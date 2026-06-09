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
#include "../Components/CurrentFrameComponent.hpp"
#include "../Components/VMAllocatorComponent.hpp"
#include "../Components/ParticlesBufferComponent.hpp"

const int MAX_NUMBER_PARTICLES = 100000;
const int TEST_SPAWN_COUNT = 100;

struct ParticleMetada
{
	glm::vec3 directionalVector;
	uint32_t bottomOfStack;
	uint32_t maxNumberOfPatricles;
	glm::vec2 spawnRadius; // 1-min, 2-max
	glm::vec2 timeToLive;  // 1-min, 2-max
	glm::vec2 velocity;    // 1-min, 2-max
	glm::vec2 scale;       // 1-min, 2-max
};

struct DispatchIndirect
{
	uint32_t x;
	uint32_t y;
	uint32_t z;
	uint32_t spawnCount;
};

struct DrawIndirect
{
	uint32_t vertexCount;
	uint32_t instanceCount;
	uint32_t firstVertex;
	uint32_t firstInstance;
};

struct ParticleProperties
{
	glm::vec3 position = {0.0f, 0.0f, 0.0f};
	uint32_t seed = 0;
	glm::vec3 scale = {1.0f, 1.0f, 1.0f};
	float liveTime = -1.0f;
};

struct EmitorPushConst
{
	float deltaTime;
	uint32_t totalFrames;
};

void ParticleSystemComputePass::drawParticleCompute(vk::raii::CommandBuffer& cmd, uint32_t frame,
                                                    DescriptorManagerComponent& dManager, BufferManager& bManager,
                                                    PipelineManager& pManager, uint32_t totalFrames, float deltaTime)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pManager.pipelines["particles_spawner"].pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pManager.pipelines["particles_spawner"].layout, 0,
	                       dManager.descriptorManager->getSet(_dSetParticles, frame), nullptr);

	if (totalFrames % 2 == 0)
	{
		cmd.fillBuffer(bManager.buffers[_dispatchBufferForEmiterB.id].buffer[0], 0, 4,
		               0); // Nulling x
		cmd.fillBuffer(bManager.buffers[_dispatchBufferForEmiterB.id].buffer[0], 12, 4,
		               0); // Nulling spawnCount
	}
	else
	{
		cmd.fillBuffer(bManager.buffers[_dispatchBufferForEmiterA.id].buffer[0], 0, 4,
		               0); // Nulling x
		cmd.fillBuffer(bManager.buffers[_dispatchBufferForEmiterA.id].buffer[0], 12, 4,
		               0); // Nulling spawnCount
	}

	cmd.pushConstants<uint32_t>(*pManager.pipelines["particles_spawner"].layout, vk::ShaderStageFlagBits::eCompute, 0,
	                            totalFrames);

	cmd.dispatchIndirect(bManager.buffers[_dispatchBuffer.id].buffer[0], 0);

	vk::MemoryBarrier2 fillBarrier;
	fillBarrier.srcStageMask = vk::PipelineStageFlagBits2::eDrawIndirect | vk::PipelineStageFlagBits2::eComputeShader | vk::PipelineStageFlagBits2::eTransfer;
	fillBarrier.srcAccessMask = vk::AccessFlagBits2::eIndirectCommandRead | vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eTransferWrite;
	fillBarrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer | vk::PipelineStageFlagBits2::eComputeShader | vk::PipelineStageFlagBits2::eDrawIndirect;
	fillBarrier.dstAccessMask = vk::AccessFlagBits2::eTransferWrite | vk::AccessFlagBits2::eShaderWrite | vk::AccessFlagBits2::eIndirectCommandRead | vk::AccessFlagBits2::eShaderRead;

	vk::DependencyInfo fillDepInfo;
	fillDepInfo.memoryBarrierCount = 1;
	fillDepInfo.pMemoryBarriers = &fillBarrier;
	cmd.pipelineBarrier2(fillDepInfo);

	cmd.fillBuffer(bManager.buffers[_dispatchBuffer.id].buffer[0], 0, 4,
	               0); // Nulling x
	cmd.fillBuffer(bManager.buffers[_dispatchBuffer.id].buffer[0], 12, 4,
	               0); // Nulling spawnCount
	// ^ We dont want to nulling y and z ^

	cmd.fillBuffer(bManager.buffers[_indirectBuffer.id].buffer[frame], 4, 4, 0);

	vk::MemoryBarrier2 emiterBarrier;
	emiterBarrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer | vk::PipelineStageFlagBits2::eComputeShader;
	emiterBarrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite | vk::AccessFlagBits2::eShaderWrite;
	emiterBarrier.dstStageMask = vk::PipelineStageFlagBits2::eComputeShader | vk::PipelineStageFlagBits2::eDrawIndirect;
	emiterBarrier.dstAccessMask = vk::AccessFlagBits2::eShaderWrite | vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eIndirectCommandRead;

	vk::DependencyInfo emiterDepInfo;
	emiterDepInfo.memoryBarrierCount = 1;
	emiterDepInfo.pMemoryBarriers = &emiterBarrier;
	cmd.pipelineBarrier2(emiterDepInfo);

	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pManager.pipelines["particles_emiter"].pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pManager.pipelines["particles_emiter"].layout, 0,
	                       dManager.descriptorManager->getSet(_dSetParticles, frame), nullptr);

	EmitorPushConst push;
	push.deltaTime = deltaTime;
	push.totalFrames = totalFrames;

	cmd.pushConstants<EmitorPushConst>(*pManager.pipelines["particles_emiter"].layout, vk::ShaderStageFlagBits::eCompute,
	                                   0, push);

	if (totalFrames % 2 == 0)
	{
		cmd.dispatchIndirect(bManager.buffers[_dispatchBufferForEmiterA.id].buffer[0], 0);
	}
	else
	{
		cmd.dispatchIndirect(bManager.buffers[_dispatchBufferForEmiterB.id].buffer[0], 0);
	}

	vk::MemoryBarrier2 renderReadyBarrier;
	renderReadyBarrier.srcStageMask = vk::PipelineStageFlagBits2::eComputeShader;
	renderReadyBarrier.srcAccessMask = vk::AccessFlagBits2::eShaderWrite;
	renderReadyBarrier.dstStageMask =
	    vk::PipelineStageFlagBits2::eDrawIndirect | vk::PipelineStageFlagBits2::eVertexShader;
	renderReadyBarrier.dstAccessMask = vk::AccessFlagBits2::eIndirectCommandRead | vk::AccessFlagBits2::eShaderRead;

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

	_particlesBuffer = bManager.createBuffer(
	    vk::MemoryPropertyFlagBits::eHostVisible, sizeof(ParticleProperties) * MAX_NUMBER_PARTICLES, 1,
	    vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);

	_particlesStack =
	    bManager.createBuffer(vk::MemoryPropertyFlagBits::eHostVisible, sizeof(uint32_t) * MAX_NUMBER_PARTICLES, 1,
	                          vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);

	_particlesMetada =
	    bManager.createBuffer(vk::MemoryPropertyFlagBits::eHostVisible, sizeof(ParticleMetada), 1,
	                          vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);

	_dispatchBuffer =
	    bManager.createBuffer(vk::MemoryPropertyFlagBits::eDeviceLocal, sizeof(DispatchIndirect), 1,
	                          vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer |
	                              vk::BufferUsageFlagBits::eTransferDst);
	_indirectBuffer =
	    bManager.createBuffer(vk::MemoryPropertyFlagBits::eDeviceLocal, sizeof(DrawIndirect), MAX_FRAMES_IN_FLIGHT,
	                          vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer |
	                              vk::BufferUsageFlagBits::eTransferDst);
	_aliveIndicesBufferA =
	    bManager.createBuffer(vk::MemoryPropertyFlagBits::eDeviceLocal, sizeof(uint32_t) * MAX_NUMBER_PARTICLES, 1,
	                          vk::BufferUsageFlagBits::eStorageBuffer);
	_aliveIndicesBufferB =
	    bManager.createBuffer(vk::MemoryPropertyFlagBits::eDeviceLocal, sizeof(uint32_t) * MAX_NUMBER_PARTICLES, 1,
	                          vk::BufferUsageFlagBits::eStorageBuffer);

	_dispatchBufferForEmiterA =
	    bManager.createBuffer(vk::MemoryPropertyFlagBits::eDeviceLocal, sizeof(DispatchIndirect), 1,
	                          vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer |
	                              vk::BufferUsageFlagBits::eTransferDst);

	_dispatchBufferForEmiterB =
	    bManager.createBuffer(vk::MemoryPropertyFlagBits::eDeviceLocal, sizeof(DispatchIndirect), 1,
	                          vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer |
	                              vk::BufferUsageFlagBits::eTransferDst);

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		dManager.update(_dSetParticles, 0, i, vk::DescriptorType::eStorageBuffer,
		                bManager.buffers[_particlesBuffer.id].buffer[0]);
		dManager.update(_dSetParticles, 1, i, vk::DescriptorType::eStorageBuffer,
		                bManager.buffers[_particlesStack.id].buffer[0]);
		dManager.update(_dSetParticles, 2, i, vk::DescriptorType::eStorageBuffer,
		                bManager.buffers[_particlesMetada.id].buffer[0]);
		dManager.update(_dSetParticles, 3, i, vk::DescriptorType::eStorageBuffer,
		                bManager.buffers[_dispatchBuffer.id].buffer[0]);
		dManager.update(_dSetParticles, 4, i, vk::DescriptorType::eStorageBuffer,
		                bManager.buffers[_indirectBuffer.id].buffer[i]);
		dManager.update(_dSetParticles, 5, i, vk::DescriptorType::eStorageBuffer,
		                bManager.buffers[_aliveIndicesBufferA.id].buffer[0]);
		dManager.update(_dSetParticles, 6, i, vk::DescriptorType::eStorageBuffer,
		                bManager.buffers[_aliveIndicesBufferB.id].buffer[0]);
		dManager.update(_dSetParticles, 7, i, vk::DescriptorType::eStorageBuffer,
		                bManager.buffers[_dispatchBufferForEmiterA.id].buffer[0]);
		dManager.update(_dSetParticles, 8, i, vk::DescriptorType::eStorageBuffer,
		                bManager.buffers[_dispatchBufferForEmiterB.id].buffer[0]);
	}

	auto& vulkanDevice = *gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;

	std::vector<uint32_t> sequenceData(MAX_NUMBER_PARTICLES);
	for (uint32_t i = 0; i < MAX_NUMBER_PARTICLES; ++i)
	{
		sequenceData[i] = i;
	}

	StagingBuffer staging = VulkanUtils::createStagingBuffer(
	    sequenceData.data(), sizeof(uint32_t) * MAX_NUMBER_PARTICLES, allocator, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

	std::vector<ParticleProperties> sequenceDataParticles(MAX_NUMBER_PARTICLES);
	for (uint32_t i = 0; i < MAX_NUMBER_PARTICLES; ++i)
	{
		sequenceDataParticles[i].liveTime = -1.0;
	}
	StagingBuffer stagingParticles =
	    VulkanUtils::createStagingBuffer(sequenceDataParticles.data(), sizeof(ParticleProperties) * MAX_NUMBER_PARTICLES,
	                                     allocator, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

	auto cmd = VulkanUtils::beginSingleTimeCommands(vulkanDevice);

	// Stack
	vk::BufferCopy copyRegion;
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = sizeof(uint32_t) * MAX_NUMBER_PARTICLES;

	cmd.copyBuffer(vk::Buffer(staging.buffer), bManager.buffers[_particlesStack.id].buffer[0], copyRegion);

	// Particles
	vk::BufferCopy copyRegionParticles;
	copyRegionParticles.srcOffset = 0;
	copyRegionParticles.dstOffset = 0;
	copyRegionParticles.size = sizeof(ParticleProperties) * MAX_NUMBER_PARTICLES;

	cmd.copyBuffer(vk::Buffer(stagingParticles.buffer), bManager.buffers[_particlesBuffer.id].buffer[0],
	               copyRegionParticles);

	// Metadata
	ParticleMetada initialMetadata = {
	    .directionalVector = {0.0f, 1.0f, 0.0f},
	    .bottomOfStack = 0, 
	    .maxNumberOfPatricles = TEST_SPAWN_COUNT,
	    .spawnRadius = {0.0f, 5.0f},
	    .timeToLive = {1.0f, 3.0f},
	    .velocity = {1.0f, 5.0f},
	    .scale = {0.05f, 0.3f},
	};
	cmd.updateBuffer(bManager.buffers[_particlesMetada.id].buffer[0], 0,
	                 vk::ArrayProxy<const ParticleMetada>(1, &initialMetadata));

	// Indirect
	DrawIndirect initialDraw = {.vertexCount = 6, .instanceCount = 0, .firstVertex = 0, .firstInstance = 0};
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		cmd.updateBuffer(bManager.buffers[_indirectBuffer.id].buffer[i], 0,
		                 vk::ArrayProxy<const DrawIndirect>(1, &initialDraw));
	}

	// dispatch. For some reason cant use updateBuffer for this one, maybe because of the size of the buffer or
	// something, but copyBuffer works fine
	DispatchIndirect initialDispatch = {.x = TEST_SPAWN_COUNT/64, .y = 1, .z = 1, .spawnCount = TEST_SPAWN_COUNT};
	StagingBuffer stagingDispatch = VulkanUtils::createStagingBuffer(
	    &initialDispatch, sizeof(DispatchIndirect), allocator, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

	vk::BufferCopy copyRegionDispatch;
	copyRegionDispatch.srcOffset = 0;
	copyRegionDispatch.dstOffset = 0;
	copyRegionDispatch.size = sizeof(DispatchIndirect);

	cmd.copyBuffer(vk::Buffer(stagingDispatch.buffer), bManager.buffers[_dispatchBuffer.id].buffer[0], copyRegionDispatch);

	// Dispatch for emiter
	DispatchIndirect initialDispatchForEmiter = {.x = 0, .y = 1, .z = 1, .spawnCount = 0};
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		cmd.updateBuffer(bManager.buffers[_dispatchBufferForEmiterA.id].buffer[0], 0,
		                 vk::ArrayProxy<const DispatchIndirect>(1, &initialDispatchForEmiter));
	}
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		cmd.updateBuffer(bManager.buffers[_dispatchBufferForEmiterB.id].buffer[0], 0,
		                 vk::ArrayProxy<const DispatchIndirect>(1, &initialDispatchForEmiter));
	}
	VulkanUtils::endSingleTimeCommands(cmd, vulkanDevice);

	VulkanUtils::destroyStagingBuffer(staging, allocator);
	VulkanUtils::destroyStagingBuffer(stagingParticles, allocator);
	VulkanUtils::destroyStagingBuffer(stagingDispatch, allocator);

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
	    .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(EmitorPushConst)}},
	});

	Orhescyon::Entity e = gm.createEntity();
	gm.registerContext<ParticlesBufferContext>(e);
	gm.addComponent<ParticlesBufferComponent>(e, _particlesBuffer, _indirectBuffer, _aliveIndicesBufferA, _aliveIndicesBufferB);
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
