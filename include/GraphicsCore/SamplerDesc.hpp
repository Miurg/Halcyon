#pragma once

#include "HalcyonExport.hpp"
#include <cstddef>
#include <functional>

enum class SamplerFilter
{
	Nearest,
	Linear
};

enum class SamplerMipmapMode
{
	Nearest,
	Linear
};

enum class SamplerAddressMode
{
	Repeat,
	MirroredRepeat,
	ClampToEdge,
	ClampToBorder,
	MirrorClampToEdge
};

enum class SamplerBorderColor
{
	IntOpaqueBlack,
	IntOpaqueWhite,
	IntTransparentBlack,
	FloatOpaqueBlack,
	FloatOpaqueWhite,
	FloatTransparentBlack
};

// None disables depth comparison; any other value enables it with that operator.
enum class SamplerCompareOp
{
	None,
	Never,
	Less,
	Equal,
	LessOrEqual,
	Greater,
	NotEqual,
	GreaterOrEqual,
	Always
};

// The builder clamps it down to the hardware limit.
inline constexpr float SamplerAnisotropyMax = 16;

struct HALCYON_API SamplerDesc
{
	SamplerFilter magFilter = SamplerFilter::Linear;
	SamplerFilter minFilter = SamplerFilter::Linear;
	SamplerMipmapMode mipmapMode = SamplerMipmapMode::Linear;
	SamplerAddressMode addressMode = SamplerAddressMode::Repeat;
	SamplerBorderColor borderColor = SamplerBorderColor::IntOpaqueBlack;
	SamplerCompareOp compareOp = SamplerCompareOp::None;
	float maxAnisotropy = 1.0f; // <= 1 disables anisotropic filtering
	float mipLodBias = 0.0f;
	float minLod = 0.0f;

	bool operator==(const SamplerDesc&) const = default;
};

template <>
struct std::hash<SamplerDesc>
{
	size_t operator()(const SamplerDesc& desc) const noexcept
	{
		size_t seed = 0;
		auto combine = [&seed](size_t value) { seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2); };
		combine(static_cast<size_t>(desc.magFilter));
		combine(static_cast<size_t>(desc.minFilter));
		combine(static_cast<size_t>(desc.mipmapMode));
		combine(static_cast<size_t>(desc.addressMode));
		combine(static_cast<size_t>(desc.borderColor));
		combine(static_cast<size_t>(desc.compareOp));
		combine(std::hash<float>{}(desc.maxAnisotropy));
		combine(std::hash<float>{}(desc.mipLodBias));
		combine(std::hash<float>{}(desc.minLod));
		return seed;
	}
};

namespace samplerPresets
{
	inline SamplerDesc texture()
	{
		SamplerDesc desc;
		desc.maxAnisotropy = SamplerAnisotropyMax;
		return desc;
	}

	inline SamplerDesc cubemap()
	{
		SamplerDesc desc;
		desc.addressMode = SamplerAddressMode::ClampToEdge;
		desc.borderColor = SamplerBorderColor::FloatOpaqueWhite;
		return desc;
	}
}
