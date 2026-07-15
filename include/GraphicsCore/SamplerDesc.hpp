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
	ClampToBorder
};

enum class SamplerBorderColor
{
	IntOpaqueBlack,
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

struct HALCYON_API SamplerDesc
{
	SamplerFilter magFilter = SamplerFilter::Linear;
	SamplerFilter minFilter = SamplerFilter::Linear;
	SamplerMipmapMode mipmapMode = SamplerMipmapMode::Linear;
	SamplerAddressMode addressMode = SamplerAddressMode::Repeat;
	SamplerBorderColor borderColor = SamplerBorderColor::IntOpaqueBlack;
	SamplerCompareOp compareOp = SamplerCompareOp::None;
	bool anisotropy = false;
};

namespace samplerPresets
{
	inline SamplerDesc texture()
	{
		SamplerDesc desc;
		desc.anisotropy = true;
		return desc;
	}

	inline SamplerDesc cubemap()
	{
		SamplerDesc desc;
		desc.addressMode = SamplerAddressMode::ClampToEdge;
		desc.borderColor = SamplerBorderColor::FloatOpaqueWhite;
		desc.anisotropy = true;
		return desc;
	}
}
