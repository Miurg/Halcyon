#pragma once

#include "HalcyonExport.hpp"
#include "DeletionQueue.hpp"

struct HALCYON_API DeletionQueueComponent
{
    DeletionQueue* queue = nullptr;

    DeletionQueueComponent() = default;
    DeletionQueueComponent(DeletionQueue* q) : queue(q) {}
};
