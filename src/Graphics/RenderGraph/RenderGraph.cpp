#include "RenderGraph.hpp"
#include "../VulkanDevice.hpp"
#include "../Resources/Managers/DescriptorManager.hpp"
#include "../Resources/Components/GlobalDSetComponent.hpp"
#include "../Resources/Managers/Bindings.hpp"
#include "../Components/GraphicsSettingsComponent.hpp"

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

RGResourceHandle RenderGraph::importImage(const std::string& name, vk::Image image, vk::ImageView imageView,
                                          vk::ImageAspectFlags aspect, vk::ImageLayout currentLayout)
{
	// Check if resource already exists by name
	for (uint32_t i = 0; i < resources.size(); ++i)
	{
		if (resources[i].name == name && !resources[i].isTransient)
		{
			resources[i].image = image;
			resources[i].imageView = imageView;
			resources[i].currentLayout = currentLayout;
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
	entry.currentLayout = currentLayout;
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
		else if (res.desc.sizeMode == RGSizeMode::QuarterExtent)
		{
			targetW /= 4;
			targetH /= 4;
		}
		else if (res.desc.sizeMode == RGSizeMode::EighthExtent)
		{
			targetW /= 8;
			targetH /= 8;
		}
		else if (res.desc.sizeMode == RGSizeMode::SixteenthExtent)
		{
			targetW /= 16;
			targetH /= 16;
		}
		else if (res.desc.sizeMode == RGSizeMode::ThirtySecondExtent)
		{
			targetW /= 32;
			targetH /= 32;
		}
		else if (res.desc.sizeMode == RGSizeMode::SixtyFourthExtent)
		{
			targetW /= 64;
			targetH /= 64;
		}

		if (res.currentWidth == targetW && res.currentHeight == targetH) continue;

		// Destroy old resources if they exist
		destroyTransientImage(res);

		// Allocate with new dimensions
		res.currentWidth = targetW;
		res.currentHeight = targetH;
		res.currentLayout = vk::ImageLayout::eUndefined;
		allocateTransientImage(res);

		// Create sampler on first allocation
		if (!res.sampler)
		{
			createSampler(res);
		}
	}

	needsDescriptorUpdate = true;
}

void RenderGraph::declareLogicalStream(const std::string& name, const RGImageDesc& desc)
{
	logicalStreams[name] = desc;
	for (auto& res : resources)
	{
		// Match exact logical name or versioned physical name (e.g. "MainColor_V0")
		if ((res.name == name || res.name.find(name + "_V") == 0) && res.isTransient)
		{
			res.desc = desc;
			destroyTransientImage(res);   // Force destruction immediately
			res.currentLayout = vk::ImageLayout::eUndefined;
			res.currentWidth = 0;         // Force recreation during next handleResize
		}
	}
}

void RenderGraph::setTerminalOutput(const std::string& logicalName, const std::string& physicalName)
{
	terminalOutputs[logicalName] = physicalName;
}

void RenderGraph::addPass(const std::string& name, const RGPassDesc& desc, std::vector<RGResourceAccess> reads,
                          std::vector<RGResourceAccess> writes,
                          std::function<void(vk::raii::CommandBuffer& cmd)> executeFn,
                          std::function<void(const RenderGraph& rg, const RGPass& pass)> updateDescriptorsFn)
{
	passes.push_back(
	    {name, desc, std::move(reads), std::move(writes), std::move(executeFn), std::move(updateDescriptorsFn)});
}

void RenderGraph::compile()
{
	compiledPasses.clear();

	// Calculate a simple hash based on passes to detect topology changes
	size_t currentHash = 0;
	auto hash_combine = [&currentHash](const std::string& v)
	{ currentHash ^= std::hash<std::string>{}(v) + 0x9e3779b9 + (currentHash << 6) + (currentHash >> 2); };
	for (const auto& pass : passes)
	{
		hash_combine(pass.name);
		for (const auto& r : pass.reads) hash_combine(r.name);
		for (const auto& w : pass.writes) hash_combine(w.name);
	}

	// If the hash differs from the last compiled version, we need to update descriptors (bindings might have changed)
	if (currentHash != lastPassHash)
	{
		lastPassHash = currentHash;
		needsDescriptorUpdate = true;
	}

	// Helper to get or create transient resource
	auto getOrCreateResourceHnd = [&](const std::string& physicalName,
	                                  const std::string& logicalName) -> RGResourceHandle
	{
		for (uint32_t i = 0; i < resources.size(); ++i)
		{
			if (resources[i].name == physicalName) return i;
		}

		auto it = logicalStreams.find(logicalName);
		if (it == logicalStreams.end() && physicalName != logicalName)
		{
			throw std::runtime_error("RenderGraph: Logical stream not declared: " + logicalName);
		}
		RGImageDesc desc = (it != logicalStreams.end()) ? it->second : RGImageDesc{};

		RGResourceHandle handle = static_cast<RGResourceHandle>(resources.size());
		RGResourceEntry entry;
		entry.name = physicalName;
		entry.aspectFlags = desc.aspectFlags;
		entry.isTransient = true;
		entry.desc = desc;
		entry.currentWidth = currentWidth;
		entry.currentHeight = currentHeight;
		if (desc.sizeMode == RGSizeMode::HalfExtent)
		{
			entry.currentWidth /= 2;
			entry.currentHeight /= 2;
		}
		else if (desc.sizeMode == RGSizeMode::QuarterExtent)
		{
			entry.currentWidth /= 4;
			entry.currentHeight /= 4;
		}
		else if (desc.sizeMode == RGSizeMode::EighthExtent)
		{
			entry.currentWidth /= 8;
			entry.currentHeight /= 8;
		}
		else if (desc.sizeMode == RGSizeMode::SixteenthExtent)
		{
			entry.currentWidth /= 16;
			entry.currentHeight /= 16;
		}
		else if (desc.sizeMode == RGSizeMode::ThirtySecondExtent)
		{
			entry.currentWidth /= 32;
			entry.currentHeight /= 32;
		}
		else if (desc.sizeMode == RGSizeMode::SixtyFourthExtent)
		{
			entry.currentWidth /= 64;
			entry.currentHeight /= 64;
		}
		resources.push_back(std::move(entry));
		allocateTransientImage(resources.back());
		createSampler(resources.back());
		return handle;
	};

	// === 1. Reverse Pass ===
	std::unordered_map<std::string, std::string> requiredOut = terminalOutputs;
	std::unordered_map<std::string, uint32_t> versionCounter;
	std::vector<std::unordered_map<std::string, std::string>> passLogicalToPhysicalWrites(passes.size());
	std::vector<std::unordered_map<std::string, std::string>> passLogicalToPhysicalReads(passes.size());

	for (int i = static_cast<int>(passes.size()) - 1; i >= 0; --i)
	{
		auto& pass = passes[i];

		// Check what this pass writes
		for (const auto& write : pass.writes)
		{
			if (!requiredOut.count(write.name))
			{
				requiredOut[write.name] = write.name + "_V" + std::to_string(versionCounter[write.name]++);
			}
			passLogicalToPhysicalWrites[i][write.name] = requiredOut[write.name];
		}

		// Handle reads: if a pass READS what it WRITES (ping-pong), the preceding pass MUST output a different physical
		// version! Also, register what physical version the pass expects to read.
		for (const auto& read : pass.reads)
		{
			bool isPingPong = false;
			for (const auto& write : pass.writes)
			{
				if (write.name == read.name)
				{
					isPingPong = true;
					break;
				}
			}

			if (isPingPong || !requiredOut.count(read.name))
			{
				requiredOut[read.name] = read.name + "_V" + std::to_string(versionCounter[read.name]++);
			}
			passLogicalToPhysicalReads[i][read.name] = requiredOut[read.name];
		}
	}

	// === 2. Forward Pass ===
	std::unordered_map<std::string, std::string> currentState;
	for (uint32_t i = 0; i < resources.size(); ++i)
	{
		currentState[resources[i].name] = resources[i].name; // identity mapping for imported resources
	}

	for (size_t i = 0; i < passes.size(); ++i)
	{
		auto& pass = passes[i];
		pass.resolvedReads.clear();
		pass.resolvedWrites.clear();

		for (const auto& read : pass.reads)
		{
			std::string physical =
			    currentState.count(read.name) ? currentState[read.name] : read.name; // fallback to identity
			pass.resolvedReads[read.name] = getOrCreateResourceHnd(physical, read.name);
		}
		for (const auto& write : pass.writes)
		{
			std::string physical = passLogicalToPhysicalWrites[i][write.name];
			RGResourceHandle handle = getOrCreateResourceHnd(physical, write.name);
			pass.resolvedWrites[write.name] = handle;
			currentState[write.name] = physical;
		}
	}

	resourceStates.resize(resources.size());
	for (uint32_t i = 0; i < resources.size(); ++i)
	{
		resourceStates[i] = RGResourceState{};
		resourceStates[i].layout = resources[i].currentLayout;
	}

	// With all logical names resolved to physical ones, we can determine barriers and final layouts for each pass.
	for (auto& pass : passes)
	{
		RGCompiledPass compiled;
		compiled.pass = &pass;

		auto processAccess = [&](RGResourceHandle handle, RGResourceUsage usage)
		{
			if (handle >= resources.size() || handle == RG_INVALID_HANDLE) return;

			RGResourceState& state = resourceStates[handle];
			vk::ImageLayout requiredLayout = usageToLayout(usage);
			vk::AccessFlags2 requiredAccess = usageToAccessMask(usage);
			vk::PipelineStageFlags2 dstStage = usageToDstStageMask(usage);

			if (state.layout != requiredLayout || state.accessMask != requiredAccess)
			{
				RGBarrier barrier;
				barrier.image = resources[handle].image;
				barrier.oldLayout = state.layout;
				barrier.newLayout = requiredLayout;
				barrier.srcAccessMask = state.accessMask;
				barrier.dstAccessMask = requiredAccess;
				barrier.srcStageMask = state.stageMask;
				barrier.dstStageMask = dstStage;
				barrier.aspectFlags = resources[handle].aspectFlags;
				compiled.barriers.push_back(barrier);

				resources[handle].currentLayout = requiredLayout;
			}

			state.layout = requiredLayout;
			state.accessMask = requiredAccess;
			state.stageMask = usageToCompleteStageMask(usage);
		};

		for (const auto& read : pass.reads) processAccess(pass.resolvedReads[read.name], read.usage);
		for (const auto& write : pass.writes) processAccess(pass.resolvedWrites[write.name], write.usage);

		compiledPasses.push_back(std::move(compiled));
	}

	// Now that everything is mapped, update descriptors if necessary
	if (needsDescriptorUpdate)
	{
		(*vulkanDevice.device).waitIdle();
		for (const auto& pass : passes)
		{
			if (pass.updateDescriptors)
			{
				pass.updateDescriptors(*this, pass);
			}
		}
		needsDescriptorUpdate = false;
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

		// If this pass has rendering attachments, begin a dynamic render pass. Otherwise, it's a compute/dispatch-only
		// pass.
		if (!desc.isCompute && (!desc.colorAttachments.empty() || desc.depthAttachment.has_value()))
		{
			std::vector<vk::RenderingAttachmentInfo> colorInfos;
			for (const auto& att : desc.colorAttachments)
			{
				RGResourceHandle handle = compiled.pass->getPhysicalWrite(att.name);
				if (handle == RG_INVALID_HANDLE) continue;

				vk::RenderingAttachmentInfo info;
				info.imageView = resources[handle].imageView;
				info.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
				info.loadOp = att.loadOp;
				info.storeOp = att.storeOp;
				info.clearValue = att.clearValue;

				if (!att.resolveTarget.empty())
				{
					RGResourceHandle resolveHnd = compiled.pass->getPhysicalWrite(att.resolveTarget);
					if (resolveHnd != RG_INVALID_HANDLE)
					{
						info.resolveMode = att.resolveMode == vk::ResolveModeFlagBits::eNone ? vk::ResolveModeFlagBits::eAverage : att.resolveMode;
						info.resolveImageView = resources[resolveHnd].imageView;
						info.resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal;
					}
				}

				colorInfos.push_back(info);
			}

			vk::RenderingAttachmentInfo depthInfo;
			if (desc.depthAttachment.has_value())
			{
				const auto& da = desc.depthAttachment.value();
				RGResourceHandle handle = compiled.pass->getPhysicalWrite(da.name);
				if (handle == RG_INVALID_HANDLE) handle = compiled.pass->getPhysicalRead(da.name); // try read
				if (handle != RG_INVALID_HANDLE)
				{
					depthInfo.imageView = resources[handle].imageView;
					depthInfo.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
					depthInfo.loadOp = da.loadOp;
					depthInfo.storeOp = da.storeOp;
					depthInfo.clearValue = da.clearValue;

					if (!da.resolveTarget.empty())
					{
						RGResourceHandle resolveHnd = compiled.pass->getPhysicalWrite(da.resolveTarget);
						if (resolveHnd != RG_INVALID_HANDLE)
						{
							depthInfo.resolveMode = da.resolveMode == vk::ResolveModeFlagBits::eNone ? vk::ResolveModeFlagBits::eSampleZero : da.resolveMode;
							depthInfo.resolveImageView = resources[resolveHnd].imageView;
							depthInfo.resolveImageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
						}
					}
				}
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
					firstHandle = compiled.pass->getPhysicalWrite(desc.colorAttachments[0].name);
				else if (desc.depthAttachment.has_value())
				{
					firstHandle = compiled.pass->getPhysicalWrite(desc.depthAttachment->name);
					if (firstHandle == RG_INVALID_HANDLE)
						firstHandle = compiled.pass->getPhysicalRead(desc.depthAttachment->name);
				}

				if (firstHandle != RG_INVALID_HANDLE)
				{
					const auto& res = resources[firstHandle];
					if (res.isTransient && res.currentWidth > 0)
						extent = vk::Extent2D{res.currentWidth, res.currentHeight};
					else
						extent = vk::Extent2D{currentWidth, currentHeight};
				}
				else
				{
					extent = vk::Extent2D{currentWidth, currentHeight};
				}
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
	imageInfo.samples = static_cast<VkSampleCountFlagBits>(res.desc.samples);

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
		samplerInfo.compareEnable = vk::True;
		samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
	}
	else
	{
		// Color sampler
		samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToBorder;
		samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToBorder;
		samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToBorder;
		samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueBlack;
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
