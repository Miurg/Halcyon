#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/VulkanDevice.hpp"

struct HALCYON_API VulkanDeviceComponent
{
    VulkanDevice* vulkanDeviceInstance;

    VulkanDeviceComponent(VulkanDevice* vulkanDevice) 
        : vulkanDeviceInstance(vulkanDevice) {}

};