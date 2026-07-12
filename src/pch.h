#pragma once

#include "ThirdPartyConfig.hpp"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define GLFW_INCLUDE_NONE
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