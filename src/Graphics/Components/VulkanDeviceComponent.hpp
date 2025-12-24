#pragma once

#include <memory>
#include "../VulkanDevice.hpp"

struct VulkanDeviceComponent
{
    std::unique_ptr<VulkanDevice> vulkanDeviceInstance;

    VulkanDeviceComponent() 
        : vulkanDeviceInstance(std::make_unique<VulkanDevice>()) 
    {
    }

    ~VulkanDeviceComponent() = default;
    
    // Disable copy semantics
    VulkanDeviceComponent(const VulkanDeviceComponent&) = delete;
    VulkanDeviceComponent& operator=(const VulkanDeviceComponent&) = delete;
	
    // Enable move semantics
    VulkanDeviceComponent(VulkanDeviceComponent&&) = default;
    VulkanDeviceComponent& operator=(VulkanDeviceComponent&&) = default;
};