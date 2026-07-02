#include "GTAOPass.hpp"

#include <bit>

#include <Orhescyon/GeneralManager.hpp>

#include "GraphicsCore/GraphicsContexts.hpp"
#include "GraphicsCore/SwapChain.hpp"
#include "GraphicsCore/VulkanDevice.hpp"
#include "GraphicsCore/Components/SwapChainComponent.hpp"
#include "GraphicsCore/Components/VulkanDeviceComponent.hpp"
#include "GraphicsCore/Components/VMAllocatorComponent.hpp"
#include "GraphicsCore/Components/TextureManagerComponent.hpp"
#include "GraphicsCore/Components/DescriptorManagerComponent.hpp"
#include "GraphicsCore/Components/PipelineManagerComponent.hpp"
#include "GraphicsCore/Components/GraphicsSettingsComponent.hpp"
#include "GraphicsCore/Components/GtaoSettingsComponent.hpp"
#include "GraphicsCore/Components/RenderGraphComponent.hpp"
#include "GraphicsCore/Resources/Components/GlobalDSetComponent.hpp"
#include "GraphicsCore/Resources/Managers/TextureManager.hpp"
#include "GraphicsCore/Resources/Managers/DescriptorManager.hpp"
#include "GraphicsCore/Resources/Factories/TextureUploader.hpp"
#include "GraphicsCore/Managers/PipelineManager.hpp"
#include "GraphicsCore/Factories/PipelineFactory.hpp"
#include "GraphicsCore/RenderGraph/RenderGraph.hpp"
#include "../Resources/Data/GtaoNoiseTexture.hpp"

namespace
{
namespace GtaoBinding
{
enum : uint32_t
{
	DepthInput   = 0,
	NormalsInput = 1,
	NoiseInput   = 2,
};
}
namespace BlurBinding
{
enum : uint32_t
{
	GtaoInput    = 0,
	DepthInput   = 1,
	NormalsInput = 2,
};
}
}

bool GTAOPass::isEnabled(Orhescyon::GeneralManager& gm) const
{
	return gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>()->enableGtao;
}

void GTAOPass::onInit(Orhescyon::GeneralManager& gm)
{
	auto& dManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& tManager = *gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	auto& vulkanDevice = *gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	auto& rg = *gm.getContextComponent<RenderGraphContext, RenderGraphComponent>()->renderGraph;
	VmaAllocator allocator = gm.getContextComponent<VMAllocatorContext, VMAllocatorComponent>()->allocator;

	rg.declareLogicalStream("GTAOTexture",
	                        {vk::Format::eR8Unorm, RGSizeMode::FullExtent, vk::ImageAspectFlagBits::eColor});

	_gtaoDset  = dManager.allocate("screenSpaceSet");
	_blurHDset = dManager.allocate("screenSpaceSet");
	_blurVDset = dManager.allocate("screenSpaceSet");

	pManager.build(PipelineDescription{
	    .shaderPath = "gtao.spv",
	    .cullMode = vk::CullModeFlagBits::eNone,
	    .colorAttachments = {PipelineFactory::opaqueAttachment()},
	    .colorFormats = {vk::Format::eR8Unorm},
	    .setLayoutNames = {"screenSpaceSet", "globalSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eFragment, 0, 56u}},
	});
	pManager.build(PipelineDescription{
	    .shaderPath = "gtao_blur.spv",
	    .cullMode = vk::CullModeFlagBits::eNone,
	    .colorAttachments = {PipelineFactory::opaqueAttachment()},
	    .colorFormats = {vk::Format::eR8Unorm},
	    .setLayoutNames = {"screenSpaceSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eFragment, 0, sizeof(float) * 5}},
	});

	// Noise texture (static, 64x64, lives for the entire app — sampled per pixel by gtao.slang)
	int noiseSlot = tManager.allocateTextureSlot();
	Texture& noiseTexture = tManager.textures[noiseSlot];
	tManager.createImage(64, 64, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal,
	                     vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst |
	                         vk::ImageUsageFlagBits::eSampled,
	                     VMA_MEMORY_USAGE_AUTO, noiseTexture, 1);
	tManager.createImageView(noiseTexture, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);

	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.magFilter = vk::Filter::eNearest;
	samplerInfo.minFilter = vk::Filter::eNearest;
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;
	samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
	samplerInfo.anisotropyEnable = vk::False;
	samplerInfo.compareOp = vk::CompareOp::eAlways;
	samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;
	noiseTexture.textureSampler = (*vulkanDevice.device).createSampler(samplerInfo);

	_noiseTexture.id = noiseSlot;
	TextureUploader::uploadTextureFromBuffer(Halcyon::Graphics::Data::GTAO_NOISE_PIXELS,
	                                         static_cast<int>(Halcyon::Graphics::Data::GTAO_NOISE_WIDTH),
	                                         static_cast<int>(Halcyon::Graphics::Data::GTAO_NOISE_HEIGHT),
	                                         tManager.textures[_noiseTexture.id], allocator, vulkanDevice);

	dManager.updateSingleTextureDSet(_gtaoDset, GtaoBinding::NoiseInput,
	                                 tManager.textures[_noiseTexture.id].textureImageView,
	                                 tManager.textures[_noiseTexture.id].textureSampler);
}

void GTAOPass::drawGtao(vk::raii::CommandBuffer& cmd, SwapChain& swapChain, DescriptorManagerComponent& dManager,
                        DSetHandle gtaoDSet, DSetHandle globalDSet, const GtaoSettingsComponent& gtaoSettings,
                        PipelineManager& pManager)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pManager.pipelines["gtao"].pipeline);

	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
	                                static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pManager.pipelines["gtao"].layout, 0,
	                       dManager.descriptorManager->getSet(gtaoDSet), nullptr);

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pManager.pipelines["gtao"].layout, 1,
	                       dManager.descriptorManager->getSet(globalDSet), nullptr);

	struct GtaoPushConstants
	{
		int kernelSize;
		float radius;
		float bias;
		float power;
		int numDirections;
		float maxScreenRadius;
		float fadeStart;
		float fadeEnd;
		float texelSize[2];
		int maxMip;
		float mipBias;
		float multiBounceAlbedo;
		float thicknessScale;
	} push;
	push.kernelSize = gtaoSettings.kernelSize;
	push.radius = gtaoSettings.radius;
	push.bias = gtaoSettings.bias;
	push.power = gtaoSettings.power;
	push.numDirections = gtaoSettings.numDirections;
	push.maxScreenRadius = gtaoSettings.maxScreenRadius;
	push.fadeStart = gtaoSettings.fadeStart;
	push.fadeEnd = gtaoSettings.fadeEnd;
	push.texelSize[0] = 1.0f / static_cast<float>(swapChain.swapChainExtent.width);
	push.texelSize[1] = 1.0f / static_cast<float>(swapChain.swapChainExtent.height);

	// Index of the last pyramid mip; sampler clamps maxLod as a backstop.
	push.maxMip = static_cast<int>(std::bit_width(std::max(swapChain.swapChainExtent.width,
	                                                       swapChain.swapChainExtent.height))) -
	              1;
	push.mipBias = gtaoSettings.mipBias;
	push.multiBounceAlbedo = gtaoSettings.multiBounceAlbedo;
	push.thicknessScale = gtaoSettings.thicknessScale;

	cmd.pushConstants<GtaoPushConstants>(pManager.pipelines["gtao"].layout, vk::ShaderStageFlagBits::eFragment, 0, push);
	cmd.setCullMode(vk::CullModeFlagBits::eNone);
	cmd.draw(3, 1, 0, 0);
}

void GTAOPass::drawBlur(vk::raii::CommandBuffer& cmd, SwapChain& swapChain, DescriptorManagerComponent& dManager,
                        DSetHandle blurDSet, float dirX, float dirY, const GtaoSettingsComponent& gtaoSettings,
                        PipelineManager& pManager)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["gtao_blur"].pipeline);

	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
	                                static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["gtao_blur"].layout, 0,
	                       dManager.descriptorManager->getSet(blurDSet), nullptr);
	struct BlurPushConstants
	{
		float texelSize[2];
		float direction[2];
		float depthTolerance;
	} push;
	push.texelSize[0] = 1.0f / static_cast<float>(swapChain.swapChainExtent.width);
	push.texelSize[1] = 1.0f / static_cast<float>(swapChain.swapChainExtent.height);
	push.direction[0] = dirX;
	push.direction[1] = dirY;
	push.depthTolerance = gtaoSettings.blurDepthTolerance;

	cmd.pushConstants<BlurPushConstants>(*pManager.pipelines["gtao_blur"].layout, vk::ShaderStageFlagBits::eFragment, 0,
	                                     push);
	cmd.setCullMode(vk::CullModeFlagBits::eNone);
	cmd.draw(3, 1, 0, 0);
}

void GTAOPass::addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t /*frame*/)
{
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	auto& dManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	auto& globalDSetComponent = *gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& tManager = *gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	auto& gtaoSettings = *gm.getContextComponent<GtaoSettingsContext, GtaoSettingsComponent>();

	// Static noise — imported only when GTAO is active, layout never changes
	rg.importImage("NoiseImage", tManager.textures[_noiseTexture.id].textureImage,
	               tManager.textures[_noiseTexture.id].textureImageView, vk::ImageAspectFlagBits::eColor,
	               vk::ImageLayout::eShaderReadOnlyOptimal);

	vk::ClearValue clearWhite = vk::ClearColorValue(1.0f, 1.0f, 1.0f, 1.0f);
	rg.addPass(
	    "GTAO",
	    {.colorAttachments = {{"GTAOTexture", vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore, clearWhite}}},
	    {{"DepthPyramid", RGResourceUsage::ShaderRead, 0, RG_ALL_MIPS},
	     {"ViewNormals", RGResourceUsage::ShaderRead},
	     {"NoiseImage", RGResourceUsage::ShaderRead}},
	    {{"GTAOTexture", RGResourceUsage::ColorAttachmentWrite}},
	    [&, dset = _gtaoDset](vk::raii::CommandBuffer& cmd)
	    {
		    drawGtao(cmd, swapChain, dManager, dset, globalDSetComponent.globalDSets, gtaoSettings, pManager);
	    },
	    [&dManager, dset = _gtaoDset](const RenderGraph& graph, const RGPass& pass)
	    {
		    auto depthHnd = pass.getPhysicalRead("DepthPyramid");
		    auto normHnd = pass.getPhysicalRead("ViewNormals");
		    dManager.descriptorManager->updateSingleTextureDSet(dset, GtaoBinding::DepthInput,
		                                                        graph.getImageView(depthHnd), graph.getSampler(depthHnd));
		    dManager.descriptorManager->updateSingleTextureDSet(dset, GtaoBinding::NormalsInput,
		                                                        graph.getImageView(normHnd), graph.getSampler(normHnd));
	    });

	rg.addPass(
	    "GTAOBlurH",
	    {.colorAttachments = {{"GTAOTexture", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clearWhite}}},
	    {{"GTAOTexture", RGResourceUsage::ShaderRead},
	     {"DepthPyramid", RGResourceUsage::ShaderRead, 0, 1},
	     {"ViewNormals", RGResourceUsage::ShaderRead}},
	    {{"GTAOTexture", RGResourceUsage::ColorAttachmentWrite}},
	    [&, dset = _blurHDset](vk::raii::CommandBuffer& cmd)
	    {
		    drawBlur(cmd, swapChain, dManager, dset, 1.0f, 0.0f, gtaoSettings, pManager);
	    },
	    [&dManager, dset = _blurHDset](const RenderGraph& graph, const RGPass& pass)
	    {
		    auto gtaoHnd = pass.getPhysicalRead("GTAOTexture");
		    auto depthHnd = pass.getPhysicalRead("DepthPyramid");
		    auto normHnd = pass.getPhysicalRead("ViewNormals");
		    dManager.descriptorManager->updateSingleTextureDSet(dset, BlurBinding::GtaoInput,
		                                                        graph.getImageView(gtaoHnd), graph.getSampler(gtaoHnd));
		    dManager.descriptorManager->updateSingleTextureDSet(dset, BlurBinding::DepthInput,
		                                                        graph.getImageView(depthHnd), graph.getSampler(depthHnd));
		    dManager.descriptorManager->updateSingleTextureDSet(dset, BlurBinding::NormalsInput,
		                                                        graph.getImageView(normHnd), graph.getSampler(normHnd));
	    });

	rg.addPass(
	    "GTAOBlurV",
	    {.colorAttachments = {{"GTAOTexture", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clearWhite}}},
	    {{"GTAOTexture", RGResourceUsage::ShaderRead},
	     {"DepthPyramid", RGResourceUsage::ShaderRead, 0, 1},
	     {"ViewNormals", RGResourceUsage::ShaderRead}},
	    {{"GTAOTexture", RGResourceUsage::ColorAttachmentWrite}},
	    [&, dset = _blurVDset](vk::raii::CommandBuffer& cmd)
	    {
		    drawBlur(cmd, swapChain, dManager, dset, 0.0f, 1.0f, gtaoSettings, pManager);
	    },
	    [&dManager, dset = _blurVDset](const RenderGraph& graph, const RGPass& pass)
	    {
		    auto gtaoHnd = pass.getPhysicalRead("GTAOTexture");
		    auto depthHnd = pass.getPhysicalRead("DepthPyramid");
		    auto normHnd = pass.getPhysicalRead("ViewNormals");
		    dManager.descriptorManager->updateSingleTextureDSet(dset, BlurBinding::GtaoInput,
		                                                        graph.getImageView(gtaoHnd), graph.getSampler(gtaoHnd));
		    dManager.descriptorManager->updateSingleTextureDSet(dset, BlurBinding::DepthInput,
		                                                        graph.getImageView(depthHnd), graph.getSampler(depthHnd));
		    dManager.descriptorManager->updateSingleTextureDSet(dset, BlurBinding::NormalsInput,
		                                                        graph.getImageView(normHnd), graph.getSampler(normHnd));
	    });
}
