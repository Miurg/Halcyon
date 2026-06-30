#include "ExposurePass.hpp"
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

namespace
{
enum Binding : uint32_t
{
	OffscreenInput = 0,
};
}

struct HistogramPush
{
	float minLogLum;
	float logLumRange;
	glm::uvec2 resolution;
};

struct ExposurePush
{
	float deltaTime;
	float tauUp;
	float tauDown;
	float minEV;
	float maxEV;
	float targetLuminance;
	float lowPercent;
	float highPercent;
	uint32_t pixelCount;
	float minLogLum;
	float logLumRange;
};

void ExposurePass::drawExposurePass(vk::raii::CommandBuffer& cmd, uint32_t frame, DescriptorManager& dManager,
                                    BufferManager& bManager, PipelineManager& pManager, SwapChain& swapChain, float deltaTime, AutoExposureSettingsComponent& aeSettings)
{
	cmd.fillBuffer(bManager.buffers[_histogramBuffer.id].buffer[frame], 0, sizeof(uint32_t) * 256, 0);

	vk::MemoryBarrier2 drawBarrier;
	drawBarrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
	drawBarrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
	drawBarrier.dstStageMask = vk::PipelineStageFlagBits2::eComputeShader;
	drawBarrier.dstAccessMask = vk::AccessFlagBits2::eShaderWrite | vk::AccessFlagBits2::eShaderRead;

	vk::DependencyInfo drawDepInfo;
	drawDepInfo.memoryBarrierCount = 1;
	drawDepInfo.pMemoryBarriers = &drawBarrier;
	cmd.pipelineBarrier2(drawDepInfo);

	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pManager.pipelines["histogram"].pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pManager.pipelines["histogram"].layout, 0,
	                       dManager.getSet(_dSetMainColor, frame), nullptr);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pManager.pipelines["histogram"].layout, 1,
	                       dManager.getSet(_dSetExposure, frame), nullptr);

	const float minLogLum = aeSettings.minEV;
	const float logLumRange = aeSettings.maxEV - aeSettings.minEV;

	HistogramPush hPush{};
	hPush.minLogLum = minLogLum;
	hPush.logLumRange = logLumRange;
	hPush.resolution = {swapChain.swapChainExtent.width, swapChain.swapChainExtent.height};

	cmd.pushConstants<HistogramPush>(*pManager.pipelines["histogram"].layout, vk::ShaderStageFlagBits::eCompute, 0,
	                                 hPush);

	uint32_t gx = (swapChain.swapChainExtent.width + 15) / 16;
	uint32_t gy = (swapChain.swapChainExtent.height + 15) / 16;
	cmd.dispatch(gx, gy, 1);

	drawBarrier.srcStageMask = vk::PipelineStageFlagBits2::eComputeShader;
	drawBarrier.srcAccessMask = vk::AccessFlagBits2::eShaderWrite;
	drawBarrier.dstStageMask = vk::PipelineStageFlagBits2::eComputeShader;
	drawBarrier.dstAccessMask = vk::AccessFlagBits2::eShaderWrite | vk::AccessFlagBits2::eShaderRead;

	cmd.pipelineBarrier2(drawDepInfo);

	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pManager.pipelines["exposure"].pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pManager.pipelines["exposure"].layout, 0,
	                       dManager.getSet(_dSetExposure, frame), nullptr);

	ExposurePush ePush{};
	ePush.deltaTime = std::min(deltaTime, 0.05f);
	ePush.tauUp = aeSettings.tauUp;
	ePush.tauDown = aeSettings.tauDown;
	ePush.minEV = aeSettings.minEV;
	ePush.maxEV = aeSettings.maxEV;
	float targetLumLinear = aeSettings.targetLuminance;
	ePush.targetLuminance = log2(std::max(targetLumLinear, 1e-6f));
	ePush.lowPercent = aeSettings.lowPercent;
	ePush.highPercent = aeSettings.highPercent;
	ePush.pixelCount = hPush.resolution.x * hPush.resolution.y;
	ePush.minLogLum = hPush.minLogLum;
	ePush.logLumRange = hPush.logLumRange;

	cmd.pushConstants<ExposurePush>(*pManager.pipelines["exposure"].layout, vk::ShaderStageFlagBits::eCompute, 0, ePush);

	cmd.dispatch(1, 1, 1);

	drawBarrier.srcStageMask = vk::PipelineStageFlagBits2::eComputeShader;
	drawBarrier.srcAccessMask = vk::AccessFlagBits2::eShaderWrite;
	drawBarrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
	drawBarrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
	cmd.pipelineBarrier2(drawDepInfo);
}

void ExposurePass::onInit(Orhescyon::GeneralManager& gm)
{
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& bManager = *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	auto& dManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;

	_dSetExposure = dManager.allocate("exposureSet", MAX_FRAMES_IN_FLIGHT);
	_dSetMainColor = dManager.allocate("screenSpaceSet", MAX_FRAMES_IN_FLIGHT);

	_histogramBuffer =
	    bManager.createBuffer(vk::MemoryPropertyFlagBits::eDeviceLocal, sizeof(uint32_t) * 256, MAX_FRAMES_IN_FLIGHT,
	                          vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);
	_exposureBuffer = bManager.createBuffer(vk::MemoryPropertyFlagBits::eDeviceLocal, sizeof(float), 1,
	                          vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);

	auto& vulkanDevice = *gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	float initialExposure = 1.0f;
	auto cmd = VulkanUtils::beginSingleTimeCommands(vulkanDevice);

	cmd.updateBuffer(bManager.buffers[_exposureBuffer.id].buffer[0], 0,
	                 vk::ArrayProxy<const float>(1, &initialExposure));

	VulkanUtils::endSingleTimeCommands(cmd, vulkanDevice);

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		dManager.update(_dSetExposure, 0, i, vk::DescriptorType::eStorageBuffer,
		                bManager.buffers[_histogramBuffer.id].buffer[i]);
		dManager.update(_dSetExposure, 1, i, vk::DescriptorType::eStorageBuffer,
		                bManager.buffers[_exposureBuffer.id].buffer[0]);
	}

	pManager.build(PipelineDescription{
	    .isCompute = true,
	    .shaderPath = "histogram.spv",
	    .setLayoutNames = {"screenSpaceSet", "exposureSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(HistogramPush)}},
	});

	pManager.build(PipelineDescription{
	    .isCompute = true,
	    .shaderPath = "exposure.spv",
	    .setLayoutNames = {"exposureSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(ExposurePush)}},
	});

	Orhescyon::Entity e = gm.createEntity();
	gm.registerContext<ExposureBufferContext>(e);
	gm.addComponent<ExposureBufferComponent>(e, _exposureBuffer);
}

bool ExposurePass::isEnabled(Orhescyon::GeneralManager& gm) const
{
	return gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>()->enableAutoExposure;
}

void ExposurePass::addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame)
{
	auto& dManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
	auto& bManager = *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	float deltaTime = gm.getContextComponent<DeltaTimeContext, DeltaTimeComponent>()->deltaTime;
	auto& aeSettings = *gm.getContextComponent<GraphicsSettingsContext, AutoExposureSettingsComponent>();

	std::vector<RGResourceAccess> reads{ { "MainColor", RGResourceUsage::ShaderRead } };

	rg.addPass(
	    "Exposure", {.isCompute = true}, reads, {}, [&, frame, deltaTime](vk::raii::CommandBuffer& cmd)
	    { drawExposurePass(cmd, frame, dManager, bManager, pManager, swapChain, deltaTime, aeSettings); },
	           [&dManager, dSetMainColor = _dSetMainColor](const RenderGraph& graph, const RGPass& pass)
	           {
		           auto colorHnd = pass.getPhysicalRead("MainColor");
		           dManager.updateSingleTextureDSet(dSetMainColor, Binding::OffscreenInput,
		                                                               graph.getImageView(colorHnd), graph.getSampler(colorHnd));
	           });
}
