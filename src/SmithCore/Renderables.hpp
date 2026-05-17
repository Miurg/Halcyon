#pragma once

#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Entitys/Entity.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Smith::Renderables
{
void forgeTransform(Orhescyon::GeneralManager& gm, Orhescyon::Entity e, glm::vec3 pos, glm::quat rot);
} // namespace Smith::Renderables