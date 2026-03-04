#pragma once

#include "RGPass.hpp"
#include "RGResource.hpp"
#include <vector>

struct VulkanDevice;
struct GlobalDSetComponent;
class DescriptorManager;

// Tracks the last known state of each resource for barrier computation.
struct RGResourceState
{
	vk::ImageLayout layout = vk::ImageLayout::eUndefined;
	vk::AccessFlags2 accessMask = {};
	vk::PipelineStageFlags2 stageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
};

// Barrier to be inserted before a pass.
struct RGBarrier
{
	vk::Image image;
	vk::ImageLayout oldLayout;
	vk::ImageLayout newLayout;
	vk::AccessFlags2 srcAccessMask;
	vk::AccessFlags2 dstAccessMask;
	vk::PipelineStageFlags2 srcStageMask;
	vk::PipelineStageFlags2 dstStageMask;
	vk::ImageAspectFlags aspectFlags;
};

// Compiled pass — the pass itself plus any barriers that must precede it.
struct RGCompiledPass
{
	const RGPass* pass = nullptr;
	std::vector<RGBarrier> barriers;
};

// Render Graph: manages transient resources, computes barriers, executes passes.
class RenderGraph
{
public:
	RenderGraph(VulkanDevice& device, VmaAllocator allocator);
	~RenderGraph();

	// --- Resource API ---

	// Create a transient resource (VMA-allocated image + imageView + sampler).
	RGResourceHandle createResource(const RGImageDesc& desc, const std::string& name);

	// Import an external resource (swapchain image, shadow map — not owned by RG).
	RGResourceHandle importImage(const std::string& name, vk::Image image, vk::ImageAspectFlags aspect);

	// Resize all transient resources to match the new extent.
	void handleResize(uint32_t newWidth, uint32_t newHeight);

	// Update descriptor sets after resize/creation. Call once after handleResize.
	void updateDescriptors(DescriptorManager& dManager, GlobalDSetComponent& globalDSets);

	// --- Per-frame API ---

	void addPass(const std::string& name, std::vector<RGResourceAccess> reads, std::vector<RGResourceAccess> writes,
	             std::function<void(vk::raii::CommandBuffer& cmd)> executeFn);
	void compile();
	void execute(vk::raii::CommandBuffer& cmd);

	// Clear passes for next frame. Does NOT destroy resources.
	void clearFrame();

	// --- Accessors ---

	vk::Image getImage(RGResourceHandle handle) const;
	vk::ImageView getImageView(RGResourceHandle handle) const;
	vk::Sampler getSampler(RGResourceHandle handle) const;
	const RGImageDesc& getDesc(RGResourceHandle handle) const;
	RGResourceHandle getHandle(const std::string& name) const;

private:
	void allocateTransientImage(RGResourceEntry& res);
	void destroyTransientImage(RGResourceEntry& res);
	void createSampler(RGResourceEntry& res);

	static vk::ImageLayout usageToLayout(RGResourceUsage usage);
	static vk::AccessFlags2 usageToAccessMask(RGResourceUsage usage);
	static vk::PipelineStageFlags2 usageToDstStageMask(RGResourceUsage usage);
	static vk::PipelineStageFlags2 usageToCompleteStageMask(RGResourceUsage usage);

	VulkanDevice& vulkanDevice;
	VmaAllocator allocator;
	bool needsDescriptorUpdate = false;

	std::vector<RGResourceEntry> resources;
	std::vector<RGPass> passes;
	std::vector<RGCompiledPass> compiledPasses;
	std::vector<RGResourceState> resourceStates;
};
