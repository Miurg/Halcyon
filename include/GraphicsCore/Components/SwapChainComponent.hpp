#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/SwapChain.hpp"

struct HALCYON_API SwapChainComponent
{
	SwapChain* swapChainInstance;

	SwapChainComponent(SwapChain* swapChain) : swapChainInstance(swapChain) {}
};