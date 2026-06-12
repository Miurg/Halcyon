#pragma once

#include "DeletionQueue.hpp"

struct DeletionQueueComponent
{
    DeletionQueue* queue = nullptr;

    DeletionQueueComponent() = default;
    DeletionQueueComponent(DeletionQueue* q) : queue(q) {}
};
