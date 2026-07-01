#pragma once

#ifdef TRACY_ENABLE
#include <tracy/TracyVulkan.hpp>
#else
using TracyVkCtx = void*;
#endif

struct TracyContextComponent
{
#ifdef TRACY_ENABLE
	TracyVkCtx context = nullptr;
#else
	int dummy = 0;
#endif
};
