#pragma once

#include "HalcyonExport.hpp"
#include "PhysicsCore/Components/PhysBodyComponent.hpp"
#include "PhysicsCore/Components/PhysManagerComponent.hpp"
#include "PhysicsCore/Components/PhysTransformSnapshotComponent.hpp"
#include "PhysicsCore/Systems/PhysSnapshotSystem.hpp"
#include "PhysicsCore/PhysShapes.hpp"
#include "GraphicsCore/Systems/PhysSyncSystem.hpp"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Entitys/Entity.hpp>

namespace Smith::Phys
{
HALCYON_API void forgeBody(Orhescyon::GeneralManager& gm, Orhescyon::Entity e, Body b, Sphere sphere);
HALCYON_API void forgeBody(Orhescyon::GeneralManager& gm, Orhescyon::Entity e, Body b, Box box);
HALCYON_API void forgeBody(Orhescyon::GeneralManager& gm, Orhescyon::Entity e, Body b, Capsule capsule);
HALCYON_API void forgeBody(Orhescyon::GeneralManager& gm, Orhescyon::Entity e, Body b, Cylinder cylinder);
HALCYON_API void forgeBody(Orhescyon::GeneralManager& gm, Orhescyon::Entity e, Body b, ConvexHull convexHull);
HALCYON_API void forgeBody(Orhescyon::GeneralManager& gm, Orhescyon::Entity e, JPH::BodyID bodyID);
} // namespace Smith::Phys
