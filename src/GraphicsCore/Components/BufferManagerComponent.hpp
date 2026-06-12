#pragma once

#include "../Resources/Managers/BufferManager.hpp"

struct BufferManagerComponent
{
	BufferManager* bufferManager;

	BufferManagerComponent(BufferManager* manager) : bufferManager(manager) {}
};