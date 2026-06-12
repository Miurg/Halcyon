#include "DescriptorLayoutRegistry.hpp"
#include <stdexcept>

void DescriptorLayoutRegistry::create(const std::string& name, std::span<const vk::DescriptorSetLayoutBinding> bindings,
                                      vk::DescriptorSetLayoutCreateFlags flags, const void* pNext)
{
	vk::DescriptorSetLayoutCreateInfo info;
	info.flags = flags;
	info.bindingCount = static_cast<uint32_t>(bindings.size());
	info.pBindings = bindings.data();
	info.pNext = pNext;

	auto [it, inserted] = layouts.try_emplace(name, vk::raii::DescriptorSetLayout{device, info});
	if (!inserted)
	{
		throw std::runtime_error("DescriptorLayoutRegistry: layout '" + name + "' already registered");
	}
}

vk::DescriptorSetLayout DescriptorLayoutRegistry::get(const std::string& name) const
{
	auto it = layouts.find(name);
	if (it == layouts.end())
	{
		throw std::runtime_error("DescriptorLayoutRegistry: layout '" + name + "' not found");
	}

	return *it->second;
}

bool DescriptorLayoutRegistry::has(const std::string& name) const
{
	return layouts.count(name) > 0;
}
