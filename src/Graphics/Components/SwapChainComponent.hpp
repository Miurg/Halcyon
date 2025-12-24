#pragma once

#include <memory>
#include "../SwapChain.hpp"

struct SwapChainComponent
{
	std::unique_ptr<SwapChain> swapChainInstance;

	SwapChainComponent() : swapChainInstance(std::make_unique<SwapChain>()) {}

	~SwapChainComponent() = default;

	// Disable copy semantics
	SwapChainComponent(const SwapChainComponent&) = delete;
	SwapChainComponent& operator=(const SwapChainComponent&) = delete;

	// Enable move semantics
	SwapChainComponent(SwapChainComponent&&) = default;
	SwapChainComponent& operator=(SwapChainComponent&&) = default;
};