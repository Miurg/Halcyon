#pragma once

#include "../VulkanDevice.hpp"

struct VulkanDeviceComponent
{
    VulkanDevice* vulkanDeviceInstance;

    VulkanDeviceComponent(VulkanDevice* vulkanDevice) 
        : vulkanDeviceInstance(vulkanDevice) {}

};