#pragma once

#include "RGResource.hpp"
#include <functional>
#include <string>
#include <vector>

struct RGPass
{
	std::string name;
	std::vector<RGResourceAccess> reads;
	std::vector<RGResourceAccess> writes;
	std::function<void(vk::raii::CommandBuffer& cmd)> execute;
};
