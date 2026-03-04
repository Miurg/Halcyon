#include "RenderGraph.hpp"
#include "../VulkanDevice.hpp"
#include "../Resources/Managers/DescriptorManager.hpp"
#include "../Resources/Components/GlobalDSetComponent.hpp"
#include "../Resources/Managers/Bindings.hpp"

RenderGraph::RenderGraph(VulkanDevice& device, VmaAllocator alloc) : vulkanDevice(device), allocator(alloc) {}

RenderGraph::~RenderGraph()
{
	for (auto& res : resources)
	{
		if (res.isTransient)
		{
			destroyTransientImage(res);
			if (res.sampler)
			{
				(*vulkanDevice.device).destroySampler(res.sampler);
				res.sampler = nullptr;
			}
		}
	}
}

RGResourceHandle RenderGraph::createResource(const RGImageDesc& desc, const std::string& name)
{
	// Check if resource already exists by name
	for (uint32_t i = 0; i < resources.size(); ++i)
	{
		if (resources[i].name == name) return i;
	}

	RGResourceHandle handle = static_cast<RGResourceHandle>(resources.size());
	RGResourceEntry entry;
	entry.name = name;
	entry.aspectFlags = desc.aspectFlags;
	entry.isTransient = true;
	entry.desc = desc;
	resources.push_back(std::move(entry));
	return handle;
}

RGResourceHandle RenderGraph::importImage(const std::string& name, vk::Image image, vk::ImageView imageView,
                                          vk::ImageAspectFlags aspect)
{
	// Check if resource already exists by name
	for (uint32_t i = 0; i < resources.size(); ++i)
	{
		if (resources[i].name == name && !resources[i].isTransient)
		{
			resources[i].image = image;
			resources[i].imageView = imageView;
			return i;
		}
	}

	RGResourceHandle handle = static_cast<RGResourceHandle>(resources.size());
	RGResourceEntry entry;
	entry.name = name;
	entry.image = image;
	entry.imageView = imageView;
	entry.aspectFlags = aspect;
	entry.isTransient = false;
	resources.push_back(std::move(entry));
	return handle;
}

void RenderGraph::handleResize(uint32_t newWidth, uint32_t newHeight)
{
	currentWidth = newWidth;
	currentHeight = newHeight;

	for (auto& res : resources)
	{
		if (!res.isTransient) continue;

		uint32_t targetW = newWidth;
		uint32_t targetH = newHeight;
		if (res.desc.sizeMode == RGSizeMode::HalfExtent)
		{
			targetW /= 2;
			targetH /= 2;
		}

		if (res.currentWidth == targetW && res.currentHeight == targetH) continue;

		// Destroy old resources if they exist
		destroyTransientImage(res);

		// Allocate with new dimensions
		res.currentWidth = targetW;
		res.currentHeight = targetH;
		allocateTransientImage(res);

		// Create sampler on first allocation
		if (!res.sampler)
		{
			createSampler(res);
		}
	}

	needsDescriptorUpdate = true;
}

void RenderGraph::updateDescriptors(DescriptorManager& dManager, GlobalDSetComponent& globalDSets)
{
	if (!needsDescriptorUpdate) return;

	// Build a name→handle lookup for convenience.
	auto findRes = [&](const std::string& name) -> const RGResourceEntry*
	{
		for (const auto& r : resources)
		{
			if (r.name == name) return &r;
		}
		return nullptr;
	};

	const auto* offscreen = findRes("offscreen");
	const auto* ssaoBlur = findRes("ssaoBlur");
	const auto* depth = findRes("depth");
	const auto* viewNormals = findRes("viewNormals");
	const auto* ssao = findRes("ssao");

	// FXAA descriptor set
	if (offscreen && ssaoBlur)
	{
		dManager.updateSingleTextureDSet(globalDSets.fxaaDSets, Bindings::FXAA::ColorInput, offscreen->imageView,
		                                 offscreen->sampler);
		dManager.updateSingleTextureDSet(globalDSets.fxaaDSets, Bindings::FXAA::SsaoInput, ssaoBlur->imageView,
		                                 ssaoBlur->sampler);
		dManager.updateSingleTextureDSet(globalDSets.fxaaDSets, Bindings::FXAA::ColorInput2, offscreen->imageView,
		                                 offscreen->sampler);
	}

	// SSAO descriptor set
	if (depth && viewNormals)
	{
		dManager.updateSingleTextureDSet(globalDSets.ssaoDSets, Bindings::SSAO::DepthInput, depth->imageView,
		                                 depth->sampler);
		dManager.updateSingleTextureDSet(globalDSets.ssaoDSets, Bindings::SSAO::NormalsInput, viewNormals->imageView,
		                                 viewNormals->sampler);
		// Note: SSAO noise is NOT managed by RG (static texture from TextureManager)
	}

	// SSAO Blur descriptor set
	if (ssao && offscreen)
	{
		dManager.updateSingleTextureDSet(globalDSets.ssaoBlurDSets, Bindings::SSAOBlur::SsaoInput, ssao->imageView,
		                                 ssao->sampler);
		dManager.updateSingleTextureDSet(globalDSets.ssaoBlurDSets, Bindings::SSAOBlur::ColorInput1, offscreen->imageView,
		                                 offscreen->sampler);
		dManager.updateSingleTextureDSet(globalDSets.ssaoBlurDSets, Bindings::SSAOBlur::ColorInput2, offscreen->imageView,
		                                 offscreen->sampler);
	}

	needsDescriptorUpdate = false;
}

void RenderGraph::addPass(const std::string& name, const RGPassDesc& desc, std::vector<RGResourceAccess> reads,
                          std::vector<RGResourceAccess> writes,
                          std::function<void(vk::raii::CommandBuffer& cmd)> executeFn)
{
	passes.push_back({name, desc, std::move(reads), std::move(writes), std::move(executeFn)});
}

void RenderGraph::compile()
{
	compiledPasses.clear();
	resourceStates.assign(resources.size(), RGResourceState{});

	for (const auto& pass : passes)
	{
		RGCompiledPass compiled;
		compiled.pass = &pass;

		auto processAccess = [&](const RGResourceAccess& access)
		{
			if (access.handle >= resources.size()) return;

			RGResourceState& state = resourceStates[access.handle];
			vk::ImageLayout requiredLayout = usageToLayout(access.usage);
			vk::AccessFlags2 requiredAccess = usageToAccessMask(access.usage);
			vk::PipelineStageFlags2 dstStage = usageToDstStageMask(access.usage);

			if (state.layout != requiredLayout || state.accessMask != vk::AccessFlags2{})
			{
				RGBarrier barrier;
				barrier.image = resources[access.handle].image;
				barrier.oldLayout = state.layout;
				barrier.newLayout = requiredLayout;
				barrier.srcAccessMask = state.accessMask;
				barrier.dstAccessMask = requiredAccess;
				barrier.srcStageMask = state.stageMask;
				barrier.dstStageMask = dstStage;
				barrier.aspectFlags = resources[access.handle].aspectFlags;
				compiled.barriers.push_back(barrier);
			}

			state.layout = requiredLayout;
			state.accessMask = requiredAccess;
			state.stageMask = usageToCompleteStageMask(access.usage);
		};

		for (const auto& read : pass.reads) processAccess(read);
		for (const auto& write : pass.writes) processAccess(write);

		compiledPasses.push_back(std::move(compiled));
	}
}

void RenderGraph::execute(vk::raii::CommandBuffer& cmd)
{
	for (const auto& compiled : compiledPasses)
	{
		// Emit barriers
		if (!compiled.barriers.empty())
		{
			std::vector<vk::ImageMemoryBarrier2> vkBarriers;
			vkBarriers.reserve(compiled.barriers.size());

			for (const auto& b : compiled.barriers)
			{
				vk::ImageMemoryBarrier2 barrier;
				barrier.srcStageMask = b.srcStageMask;
				barrier.srcAccessMask = b.srcAccessMask;
				barrier.dstStageMask = b.dstStageMask;
				barrier.dstAccessMask = b.dstAccessMask;
				barrier.oldLayout = b.oldLayout;
				barrier.newLayout = b.newLayout;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.image = b.image;
				barrier.subresourceRange.aspectMask = b.aspectFlags;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = 1;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = 1;

				vkBarriers.push_back(barrier);
			}

			vk::DependencyInfo depInfo;
			depInfo.imageMemoryBarrierCount = static_cast<uint32_t>(vkBarriers.size());
			depInfo.pImageMemoryBarriers = vkBarriers.data();
			cmd.pipelineBarrier2(depInfo);
		}

		const auto& desc = compiled.pass->desc;
		bool didBeginRendering = false;

		if (!desc.isCompute && (!desc.colorAttachments.empty() || desc.depthAttachment.has_value()))
		{
			std::vector<vk::RenderingAttachmentInfo> colorInfos;
			for (const auto& att : desc.colorAttachments)
			{
				vk::RenderingAttachmentInfo info;
				info.imageView = resources[att.handle].imageView;
				info.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
				info.loadOp = att.loadOp;
				info.storeOp = att.storeOp;
				info.clearValue = att.clearValue;
				colorInfos.push_back(info);
			}

			vk::RenderingAttachmentInfo depthInfo;
			if (desc.depthAttachment.has_value())
			{
				const auto& da = desc.depthAttachment.value();
				depthInfo.imageView = resources[da.handle].imageView;
				depthInfo.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
				depthInfo.loadOp = da.loadOp;
				depthInfo.storeOp = da.storeOp;
				depthInfo.clearValue = da.clearValue;
			}

			// Auto-derive extent from first attachment dimensions if not explicitly set.
			vk::Extent2D extent;
			if (desc.customExtent.has_value())
			{
				extent = desc.customExtent.value();
			}
			else
			{
				// Use first attachment's actual dimensions.
				RGResourceHandle firstHandle = RG_INVALID_HANDLE;
				if (!desc.colorAttachments.empty())
					firstHandle = desc.colorAttachments[0].handle;
				else if (desc.depthAttachment.has_value())
					firstHandle = desc.depthAttachment->handle;

				const auto& res = resources[firstHandle];
				if (res.isTransient && res.currentWidth > 0)
					extent = vk::Extent2D{res.currentWidth, res.currentHeight};
				else
					extent = vk::Extent2D{currentWidth, currentHeight};
			}

			vk::RenderingInfo renderingInfo;
			renderingInfo.renderArea.offset = vk::Offset2D{0, 0};
			renderingInfo.renderArea.extent = extent;
			renderingInfo.layerCount = 1;
			renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorInfos.size());
			renderingInfo.pColorAttachments = colorInfos.empty() ? nullptr : colorInfos.data();
			if (desc.depthAttachment.has_value())
			{
				renderingInfo.pDepthAttachment = &depthInfo;
			}

			cmd.beginRendering(renderingInfo);
			didBeginRendering = true;
		}

		if (compiled.pass->execute)
		{
			compiled.pass->execute(cmd);
		}

		if (didBeginRendering)
		{
			cmd.endRendering();
		}
	}
}

void RenderGraph::clearFrame()
{
	passes.clear();
	compiledPasses.clear();
	resourceStates.clear();
}

// ===Accessors===

vk::Image RenderGraph::getImage(RGResourceHandle handle) const
{
	return resources[handle].image;
}

vk::ImageView RenderGraph::getImageView(RGResourceHandle handle) const
{
	return resources[handle].imageView;
}

vk::Sampler RenderGraph::getSampler(RGResourceHandle handle) const
{
	return resources[handle].sampler;
}

const RGImageDesc& RenderGraph::getDesc(RGResourceHandle handle) const
{
	return resources[handle].desc;
}

RGResourceHandle RenderGraph::getHandle(const std::string& name) const
{
	for (uint32_t i = 0; i < resources.size(); ++i)
	{
		if (resources[i].name == name) return i;
	}
	return RG_INVALID_HANDLE;
}

// ===Helpers===

void RenderGraph::allocateTransientImage(RGResourceEntry& res)
{
	vk::ImageUsageFlags usage;
	if (res.desc.aspectFlags & vk::ImageAspectFlagBits::eDepth)
	{
		usage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled;
	}
	else
	{
		usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled;
	}

	VkImageCreateInfo imageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = res.currentWidth;
	imageInfo.extent.height = res.currentHeight;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = static_cast<VkFormat>(res.desc.format);
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = static_cast<VkImageUsageFlags>(usage);
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

	VkImage rawImage;
	VkResult result = vmaCreateImage(allocator, &imageInfo, &allocInfo, &rawImage, &res.allocation, nullptr);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("RenderGraph: failed to create VMA image for '" + res.name + "'");
	}
	res.image = vk::Image(rawImage);

	vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = res.image;
	viewInfo.viewType = vk::ImageViewType::e2D;
	viewInfo.format = res.desc.format;
	viewInfo.subresourceRange.aspectMask = res.desc.aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	res.imageView = (*vulkanDevice.device).createImageView(viewInfo);
}

void RenderGraph::destroyTransientImage(RGResourceEntry& res)
{
	if (res.imageView)
	{
		(*vulkanDevice.device).destroyImageView(res.imageView);
		res.imageView = nullptr;
	}
	if (res.image && res.allocation)
	{
		vmaDestroyImage(allocator, res.image, res.allocation);
		res.image = nullptr;
		res.allocation = {};
	}
}

void RenderGraph::createSampler(RGResourceEntry& res)
{
	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.magFilter = vk::Filter::eLinear;
	samplerInfo.minFilter = vk::Filter::eLinear;
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;

	if (res.desc.aspectFlags & vk::ImageAspectFlagBits::eDepth)
	{
		// Depth sampler
		vk::PhysicalDeviceProperties properties = vulkanDevice.physicalDevice.getProperties();
		samplerInfo.anisotropyEnable = vk::True;
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.compareOp = vk::CompareOp::eAlways;
		samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
	}
	else
	{
		// Color sampler
		samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToBorder;
		samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToBorder;
		samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToBorder;
		samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueBlack;
		samplerInfo.compareEnable = vk::True;
		samplerInfo.compareOp = vk::CompareOp::eGreater;
	}

	res.sampler = (*vulkanDevice.device).createSampler(samplerInfo);
}

// ===Mappings===

vk::ImageLayout RenderGraph::usageToLayout(RGResourceUsage usage)
{
	switch (usage)
	{
	case RGResourceUsage::ColorAttachmentWrite:
		return vk::ImageLayout::eColorAttachmentOptimal;
	case RGResourceUsage::DepthAttachmentWrite:
		return vk::ImageLayout::eDepthAttachmentOptimal;
	case RGResourceUsage::DepthAttachmentRead:
		return vk::ImageLayout::eDepthReadOnlyOptimal;
	case RGResourceUsage::ShaderRead:
		return vk::ImageLayout::eShaderReadOnlyOptimal;
	case RGResourceUsage::StorageReadWrite:
		return vk::ImageLayout::eGeneral;
	case RGResourceUsage::Present:
		return vk::ImageLayout::ePresentSrcKHR;
	default:
		return vk::ImageLayout::eUndefined;
	}
}

vk::AccessFlags2 RenderGraph::usageToAccessMask(RGResourceUsage usage)
{
	switch (usage)
	{
	case RGResourceUsage::ColorAttachmentWrite:
		return vk::AccessFlagBits2::eColorAttachmentWrite;
	case RGResourceUsage::DepthAttachmentWrite:
		return vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
	case RGResourceUsage::DepthAttachmentRead:
		return vk::AccessFlagBits2::eDepthStencilAttachmentRead;
	case RGResourceUsage::ShaderRead:
		return vk::AccessFlagBits2::eShaderRead;
	case RGResourceUsage::StorageReadWrite:
		return vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite;
	case RGResourceUsage::Present:
		return vk::AccessFlags2{};
	default:
		return vk::AccessFlags2{};
	}
}

vk::PipelineStageFlags2 RenderGraph::usageToDstStageMask(RGResourceUsage usage)
{
	switch (usage)
	{
	case RGResourceUsage::ColorAttachmentWrite:
		return vk::PipelineStageFlagBits2::eColorAttachmentOutput;
	case RGResourceUsage::DepthAttachmentWrite:
		return vk::PipelineStageFlagBits2::eEarlyFragmentTests;
	case RGResourceUsage::DepthAttachmentRead:
		return vk::PipelineStageFlagBits2::eEarlyFragmentTests;
	case RGResourceUsage::ShaderRead:
		return vk::PipelineStageFlagBits2::eFragmentShader;
	case RGResourceUsage::StorageReadWrite:
		return vk::PipelineStageFlagBits2::eComputeShader;
	case RGResourceUsage::Present:
		return vk::PipelineStageFlagBits2::eBottomOfPipe;
	default:
		return vk::PipelineStageFlagBits2::eTopOfPipe;
	}
}

vk::PipelineStageFlags2 RenderGraph::usageToCompleteStageMask(RGResourceUsage usage)
{
	switch (usage)
	{
	case RGResourceUsage::ColorAttachmentWrite:
		return vk::PipelineStageFlagBits2::eColorAttachmentOutput;
	case RGResourceUsage::DepthAttachmentWrite:
		return vk::PipelineStageFlagBits2::eLateFragmentTests;
	case RGResourceUsage::DepthAttachmentRead:
		return vk::PipelineStageFlagBits2::eLateFragmentTests;
	case RGResourceUsage::ShaderRead:
		return vk::PipelineStageFlagBits2::eFragmentShader;
	case RGResourceUsage::StorageReadWrite:
		return vk::PipelineStageFlagBits2::eComputeShader;
	case RGResourceUsage::Present:
		return vk::PipelineStageFlagBits2::eBottomOfPipe;
	default:
		return vk::PipelineStageFlagBits2::eTopOfPipe;
	}
}
