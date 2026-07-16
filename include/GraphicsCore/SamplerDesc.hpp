#pragma once

#include "HalcyonExport.hpp"

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

// FromMipLevels clamps the upper LOD to the image's mip count; FullChain leaves it unbounded.
enum class SamplerMaxLod
{
	FromMipLevels,
	FullChain
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
	SamplerMaxLod maxLod = SamplerMaxLod::FromMipLevels;
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
