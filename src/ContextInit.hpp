#pragma once

#include <vulkan/vulkan_raii.hpp>

class ContextInit
{
	public:
	static void Run(vulkan::raii::Context& context)
	{
		// Currently, no specific initialization is required for vulkan::raii::Context
		// This function serves as a placeholder for future context setup if needed
	   };
}