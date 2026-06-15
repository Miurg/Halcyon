#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <span>
#include <string>
#include <unordered_map>

// A registry of named descriptor set layouts.
class DescriptorLayoutRegistry
{
public:
	DescriptorLayoutRegistry(vk::raii::Device& device) : device(device) {}

	// Create a layout and save it under a name.
	void create(const std::string& name, std::span<const vk::DescriptorSetLayoutBinding> bindings,
	            vk::DescriptorSetLayoutCreateFlags flags = {}, const void* pNext = nullptr);

	// Get a raw handle by name. Throws a runtime_error if not found.
	vk::DescriptorSetLayout get(const std::string& name) const;

	bool has(const std::string& name) const;

private:
	vk::raii::Device& device;
	std::unordered_map<std::string, vk::raii::DescriptorSetLayout> layouts;
};
