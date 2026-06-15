#pragma once

#include "HalcyonExport.hpp"
#include <deque>
#include <functional>

#include <Orhescyon/GeneralManager.hpp>

#include "GraphicsCore/Components/VulkanDeviceComponent.hpp"
#include "GraphicsCore/GraphicsContexts.hpp"

#ifdef TRACY_ENABLE
#include <tracy/TracyVulkan.hpp>
#endif

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

#ifdef TRACY_ENABLE
                if (vulkanDeviceComp->vulkanDeviceInstance->tracyContext)
                {
                    TracyVkDestroy(vulkanDeviceComp->vulkanDeviceInstance->tracyContext);
                    vulkanDeviceComp->vulkanDeviceInstance->tracyContext = nullptr;
                }
#endif
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
