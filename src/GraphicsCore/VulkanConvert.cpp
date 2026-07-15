#include "GraphicsCore/VulkanConvert.hpp"

vk::Filter VulkanConvert::toVkFilter(SamplerFilter filter)
{
	switch (filter)
	{
	case SamplerFilter::Nearest: return vk::Filter::eNearest;
	case SamplerFilter::Linear: return vk::Filter::eLinear;
	}
	return vk::Filter::eLinear;
}

vk::SamplerMipmapMode VulkanConvert::toVkMipmapMode(SamplerMipmapMode mode)
{
	switch (mode)
	{
	case SamplerMipmapMode::Nearest: return vk::SamplerMipmapMode::eNearest;
	case SamplerMipmapMode::Linear: return vk::SamplerMipmapMode::eLinear;
	}
	return vk::SamplerMipmapMode::eLinear;
}

vk::SamplerAddressMode VulkanConvert::toVkAddressMode(SamplerAddressMode mode)
{
	switch (mode)
	{
	case SamplerAddressMode::Repeat: return vk::SamplerAddressMode::eRepeat;
	case SamplerAddressMode::MirroredRepeat: return vk::SamplerAddressMode::eMirroredRepeat;
	case SamplerAddressMode::ClampToEdge: return vk::SamplerAddressMode::eClampToEdge;
	case SamplerAddressMode::ClampToBorder: return vk::SamplerAddressMode::eClampToBorder;
	}
	return vk::SamplerAddressMode::eRepeat;
}

vk::BorderColor VulkanConvert::toVkBorderColor(SamplerBorderColor color)
{
	switch (color)
	{
	case SamplerBorderColor::IntOpaqueBlack: return vk::BorderColor::eIntOpaqueBlack;
	case SamplerBorderColor::FloatOpaqueBlack: return vk::BorderColor::eFloatOpaqueBlack;
	case SamplerBorderColor::FloatOpaqueWhite: return vk::BorderColor::eFloatOpaqueWhite;
	case SamplerBorderColor::FloatTransparentBlack: return vk::BorderColor::eFloatTransparentBlack;
	}
	return vk::BorderColor::eIntOpaqueBlack;
}

vk::CompareOp VulkanConvert::toVkCompareOp(SamplerCompareOp op)
{
	switch (op)
	{
	case SamplerCompareOp::None: return vk::CompareOp::eAlways;
	case SamplerCompareOp::Never: return vk::CompareOp::eNever;
	case SamplerCompareOp::Less: return vk::CompareOp::eLess;
	case SamplerCompareOp::Equal: return vk::CompareOp::eEqual;
	case SamplerCompareOp::LessOrEqual: return vk::CompareOp::eLessOrEqual;
	case SamplerCompareOp::Greater: return vk::CompareOp::eGreater;
	case SamplerCompareOp::NotEqual: return vk::CompareOp::eNotEqual;
	case SamplerCompareOp::GreaterOrEqual: return vk::CompareOp::eGreaterOrEqual;
	case SamplerCompareOp::Always: return vk::CompareOp::eAlways;
	}
	return vk::CompareOp::eAlways;
}
