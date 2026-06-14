#pragma once

#include "GraphicsCore/SwapChain.hpp"

struct SwapChainComponent
{
	SwapChain* swapChainInstance;

	SwapChainComponent(SwapChain* swapChain) : swapChainInstance(swapChain) {}
};