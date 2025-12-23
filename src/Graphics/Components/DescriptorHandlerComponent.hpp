#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

struct DescriptorHandlerComponent
{
	DescriptorHandlerComponent(const DescriptorHandlerComponent&) = delete;
	DescriptorHandlerComponent& operator=(const DescriptorHandlerComponent&) = delete;

	DescriptorHandlerComponent(DescriptorHandlerComponent&&) = default;
	DescriptorHandlerComponent& operator=(DescriptorHandlerComponent&&) = default;

	vk::raii::DescriptorPool descriptorPool = nullptr;
	std::vector<vk::raii::DescriptorSet> descriptorSets;
};