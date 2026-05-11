#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32
#include "vk_mem_alloc.h"
#include <vector>
#include <memory>
#include <iostream>
#include <functional>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <Orhescyon/Entitys/Entity.hpp>

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif