#pragma once

#include "HalcyonExport.hpp"
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Entitys/Entity.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Smith::Renderables
{
HALCYON_API void forgeTransform(Orhescyon::GeneralManager& gm, Orhescyon::Entity e, glm::vec3 pos, glm::quat rot);
} // namespace Smith::Renderables