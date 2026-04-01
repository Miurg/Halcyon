#pragma once

#include <cstdint>
#include <string>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_raii.hpp>

using RGResourceHandle = uint32_t;
constexpr RGResourceHandle RG_INVALID_HANDLE = UINT32_MAX;

// How a pass accesses this resource
enum class RGResourceUsage
{
	ColorAttachmentWrite,
	DepthAttachmentWrite,
	DepthAttachmentRead,
	ShaderRead,
	StorageReadWrite,
	Present,
};

// A single read or write declaration for a resource in a pass
struct RGResourceAccess
{
	std::string name;
	RGResourceUsage usage = RGResourceUsage::ShaderRead;
};

// Resolution mode for transient resources
enum class RGSizeMode
{
	FullExtent,
	HalfExtent,
	QuarterExtent,
	EighthExtent,
	SixteenthExtent,
	ThirtySecondExtent,
	SixtyFourthExtent
};

// Description of a transient image resource
struct RGImageDesc
{
	vk::Format format = vk::Format::eUndefined;
	RGSizeMode sizeMode = RGSizeMode::FullExtent;
	vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor;
	vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
};

// Unified resource entry — covers both imported and transient resources
struct RGResourceEntry
{
	std::string name;
	vk::Image image = {};
	vk::ImageView imageView = {};
	vk::Sampler sampler = {};
	vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor;
	bool isTransient = false;
	vk::ImageLayout currentLayout = vk::ImageLayout::eUndefined;
	RGImageDesc desc = {};

	// VMA fields (only for transient resources)
	VmaAllocation allocation = {};
	uint32_t currentWidth = 0;
	uint32_t currentHeight = 0;
};
