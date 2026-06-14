#pragma once

#include "GraphicsCore/VulkanDevice.hpp"

struct VulkanDeviceComponent
{
    VulkanDevice* vulkanDeviceInstance;

    VulkanDeviceComponent(VulkanDevice* vulkanDevice) 
        : vulkanDeviceInstance(vulkanDevice) {}

};