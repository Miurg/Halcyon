#pragma once

#include "RGPass.hpp"
#include "RGResource.hpp"
#include <vector>
#include <unordered_map>

struct VulkanDevice;
struct GlobalDSetComponent;
struct GraphicsSettingsComponent;
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

class RenderGraph
{
public:
	RenderGraph(VulkanDevice& device, VmaAllocator allocator);
	~RenderGraph();

	// Resource API
	RGResourceHandle importImage(const std::string& name, vk::Image image, vk::ImageView imageView,
	                             vk::ImageAspectFlags aspect);
	void handleResize(uint32_t newWidth, uint32_t newHeight);

	void declareLogicalStream(const std::string& name, const RGImageDesc& desc);
	void setTerminalOutput(const std::string& logicalName, const std::string& physicalName);

	// === Per frame API === 

	// Register a render/compute pass in the graph.
	// name      - debug label for the pass.
	// desc      - rendering setup: color/depth attachments, load/store ops, clear values (see RGPassDesc).
	//             RG auto-calls beginRendering/endRendering based on this.
	//             Render area extent is auto-derived from first attachment.
	//             Set isCompute = true for dispatch-only passes (no rendering setup).
	// reads     - resources this pass samples from (triggers layout transitions).
	// writes    - resources this pass renders into (triggers layout transitions).
	// executeFn - lambda with draw/dispatch commands. Receives primary cmd buffer.
	//             Do NOT call beginRendering/endRendering inside — RG handles that.
	// updateDescriptorsFn - optional lambda to update descriptor sets before execution. Receives the entire graph and
	// the pass.
	void addPass(const std::string& name, const RGPassDesc& desc, std::vector<RGResourceAccess> reads,
	             std::vector<RGResourceAccess> writes, std::function<void(vk::raii::CommandBuffer& cmd)> executeFn,
	             std::function<void(const RenderGraph& rg, const RGPass& pass)> updateDescriptorsFn = nullptr);
	
	// Compiles the render graph. 
	// Call after adding all passes and before execution. 
	// Computes resource lifetimes, inserts barriers, etc.
	void compile();

	// Executes the compiled graph. Call after compile() and before clearFrame().
	void execute(vk::raii::CommandBuffer& cmd);

	// Clears per-frame data. Call after execute() to prepare for the next frame.
	void clearFrame();

	// === Accessors ===
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
	uint32_t currentWidth = 0;
	uint32_t currentHeight = 0;

	std::unordered_map<std::string, RGImageDesc> logicalStreams;
	std::unordered_map<std::string, std::string> terminalOutputs;

	std::vector<RGResourceEntry> resources;
	std::vector<RGPass> passes;
	std::vector<RGCompiledPass> compiledPasses;
	std::vector<RGResourceState> resourceStates;
	size_t lastPassHash = 0;
};
