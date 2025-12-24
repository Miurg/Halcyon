#pragma once

#include <memory>
#include "../SwapChain.hpp"

struct SwapChainComponent
{
	SwapChain* swapChainInstance;

	SwapChainComponent(SwapChain* swapChain) : swapChainInstance(swapChain) {}
};