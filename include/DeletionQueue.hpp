#pragma once

#include "HalcyonExport.hpp"
#include <deque>
#include <functional>

#include <Orhescyon/GeneralManager.hpp>

#include "GraphicsCore/Components/VulkanDeviceComponent.hpp"
#include "GraphicsCore/GraphicsContexts.hpp"

struct HALCYON_API DeletionQueue
{
    Orhescyon::GeneralManager* gm = nullptr;

    DeletionQueue() = default;
    DeletionQueue(Orhescyon::GeneralManager* gm) : gm(gm) {}

    std::deque<std::function<void()>> deletors;

    void push_function(std::function<void()>&& function)
    {
        deletors.push_back(std::move(function));
    }

    void flush()
    {
        if (gm)
        {
            VulkanDeviceComponent* vulkanDeviceComp =
                gm->getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>();
            if (vulkanDeviceComp && vulkanDeviceComp->vulkanDeviceInstance)
            {
                vulkanDeviceComp->vulkanDeviceInstance->device.waitIdle();
            }
        }

        for (auto it = deletors.rbegin(); it != deletors.rend(); ++it)
        {
            (*it)();
        }
        deletors.clear();
    }

    ~DeletionQueue()
    {
        flush();
    }
};
