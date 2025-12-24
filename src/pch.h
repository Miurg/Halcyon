#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define NOMINMAX 
#define WIN32_LEAN_AND_MEAN

#include <vector>
#include <memory>
#include <iostream>
#include <functional>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>