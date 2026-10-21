#pragma once

#include "HalcyonExport.hpp"
#include "Shared/Bindings.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

constexpr int MAX_FRAMES_IN_FLIGHT = 3;   // TODO: remove
constexpr int MAX_PATH_LEN = 260;         // TODO: remove
constexpr uint32_t MAX_SH_PROBES = 40960; // 127 scene probes + slot 0 (skybox fallback) TODO: remove