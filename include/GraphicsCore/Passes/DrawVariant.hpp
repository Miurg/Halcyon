#pragma once

#include "HalcyonExport.hpp"
#include <vulkan/vulkan.hpp>
#include <string_view>
#include <cstdint>

struct HALCYON_API DrawVariant
{
	std::string_view pipeline;
	vk::CullModeFlagBits cullMode;
	bool isTransparent;
};

inline const DrawVariant kDrawVariants[] = {
    {"standard_opaque", vk::CullModeFlagBits::eBack, false},
    {"standard_opaque", vk::CullModeFlagBits::eNone, false},
    {"standard_mask", vk::CullModeFlagBits::eBack, false},
    {"standard_mask", vk::CullModeFlagBits::eNone, false},
    {"standard_blend", vk::CullModeFlagBits::eBack, true},
    {"standard_blend", vk::CullModeFlagBits::eNone, true},
};
inline constexpr uint32_t kDrawVariantCount = 6;

struct HALCYON_API DrawSegment
{
	uint32_t maxCount = 0;
	uint32_t variantIndex = 0;
};
