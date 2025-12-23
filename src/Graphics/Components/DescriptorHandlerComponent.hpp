#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan_raii.hpp>
#include "../DescriptorHandler.hpp"

struct DescriptorHandlerComponent
{
	std::unique_ptr<DescriptorHandler> descriptorHandlerInstance;

	// Disable copy semantics
	DescriptorHandlerComponent(const DescriptorHandlerComponent&) = delete;
	DescriptorHandlerComponent& operator=(const DescriptorHandlerComponent&) = delete;
	// Enable move semantics
	DescriptorHandlerComponent(DescriptorHandlerComponent&&) = default;
	DescriptorHandlerComponent& operator=(DescriptorHandlerComponent&&) = default;

	DescriptorHandlerComponent(DescriptorHandler& handler)
	    : descriptorHandlerInstance(std::make_unique<DescriptorHandler>(handler))
	{
	}

	~DescriptorHandlerComponent() = default;
};