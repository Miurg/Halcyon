#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan_raii.hpp>
#include "../DescriptorHandler.hpp"

struct DescriptorHandlerComponent
{
	DescriptorHandler* descriptorHandler;

	DescriptorHandlerComponent(DescriptorHandler* descriptor) : descriptorHandler(descriptor) {}
};