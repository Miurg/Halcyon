#pragma once

#include "HalcyonExport.hpp"
#include <vulkan/vulkan.hpp>
#include <string_view>
#include <cstdint>

enum class DrawVariantIndex : uint32_t
{
	Opaque,
	OpaqueDoubleSided,
	Mask,
	MaskDoubleSided,
	Blend,
	BlendDoubleSided,
	Count
};

inline constexpr uint32_t kDrawVariantCount = static_cast<uint32_t>(DrawVariantIndex::Count);

struct HALCYON_API DrawVariant
{
	std::string_view pipeline;
	vk::CullModeFlagBits cullMode;
	bool isTransparent;
};

inline const DrawVariant kDrawVariants[kDrawVariantCount] = {
    {"standard_opaque", vk::CullModeFlagBits::eBack, false}, {"standard_opaque", vk::CullModeFlagBits::eNone, false},
    {"standard_mask", vk::CullModeFlagBits::eBack, false},   {"standard_mask", vk::CullModeFlagBits::eNone, false},
    {"standard_blend", vk::CullModeFlagBits::eBack, true},   {"standard_blend", vk::CullModeFlagBits::eNone, true},
};
struct HALCYON_API DrawSegment
{
	uint32_t maxCount = 0;
	uint32_t variantIndex = 0;
};
