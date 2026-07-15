#pragma once

#include "HalcyonExport.hpp"
#include <vulkan/vulkan_raii.hpp>
#include "GraphicsCore/SamplerDesc.hpp"

// Central hub for mechanical engine-enum -> Vulkan-enum translation.
class HALCYON_API VulkanConvert
{
public:
	static vk::Filter toVkFilter(SamplerFilter filter);
	static vk::SamplerMipmapMode toVkMipmapMode(SamplerMipmapMode mode);
	static vk::SamplerAddressMode toVkAddressMode(SamplerAddressMode mode);
	static vk::BorderColor toVkBorderColor(SamplerBorderColor color);
	static vk::CompareOp toVkCompareOp(SamplerCompareOp op);
};
