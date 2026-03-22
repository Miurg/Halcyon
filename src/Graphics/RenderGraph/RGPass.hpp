#pragma once

#include "RGResource.hpp"
#include <functional>
#include <optional>
#include <string>
#include <vector>
#include <unordered_map>

// Describes a single attachment (color or depth) for a render pass.
// handle   — RG resource to bind as attachment.
// loadOp   — what to do with contents at pass start (eClear / eLoad / eDontCare).
// storeOp  — what to do with results at pass end  (eStore / eDontCare).
// clearValue — clear color/depth if loadOp == eClear.
struct RGAttachmentConfig
{
	std::string name;
	vk::AttachmentLoadOp loadOp = vk::AttachmentLoadOp::eClear;
	vk::AttachmentStoreOp storeOp = vk::AttachmentStoreOp::eStore;
	vk::ClearValue clearValue = {};
};

// Describes the rendering setup for a pass. RG uses this to auto-call beginRendering/endRendering.
// colorAttachments — color render targets (0..N). Order matches fragment shader output locations.
// depthAttachment  — optional depth/stencil attachment.
// customExtent     — override render area size. If nullopt, auto-derived from first attachment dimensions.
//                    Only needed for external resources (e.g. shadow map with non-swapchain size).
// isCompute        — if true, skip beginRendering entirely (for compute/dispatch passes).
struct RGPassDesc
{
	std::vector<RGAttachmentConfig> colorAttachments;
	std::optional<RGAttachmentConfig> depthAttachment;
	std::optional<vk::Extent2D> customExtent;
	bool isCompute = false;
};

class RenderGraph;

struct RGPass
{
	std::string name;
	RGPassDesc desc;
	std::vector<RGResourceAccess> reads;
	std::vector<RGResourceAccess> writes;
	std::function<void(vk::raii::CommandBuffer& cmd)> execute;
	std::function<void(const RenderGraph& rg, const RGPass& pass)> updateDescriptors;

	// Internal mapping populated by RenderGraph during compilation
	std::unordered_map<std::string, RGResourceHandle> resolvedReads;
	std::unordered_map<std::string, RGResourceHandle> resolvedWrites;

	RGResourceHandle getPhysicalRead(const std::string& logicalName) const
	{
		auto it = resolvedReads.find(logicalName);
		return it != resolvedReads.end() ? it->second : RG_INVALID_HANDLE;
	}
	RGResourceHandle getPhysicalWrite(const std::string& logicalName) const
	{
		auto it = resolvedWrites.find(logicalName);
		return it != resolvedWrites.end() ? it->second : RG_INVALID_HANDLE;
	}
};
