#pragma once

#include "../PhysicsCore/Components/PhysBodyComponent.hpp"
#include "../PhysicsCore/Components/PhysManagerComponent.hpp"
#include "../PhysicsCore/Components/PhysTransformSnapshotComponent.hpp"
#include "../PhysicsCore/Systems/PhysSnapshotSystem.hpp"
#include "../PhysicsCore/PhysShapes.hpp"
#include "../Graphics/Systems/PhysSyncSystem.hpp"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Entitys/Entity.hpp>

namespace Smith::Phys
{
void forgeBody(Orhescyon::GeneralManager& gm, Orhescyon::Entity e, Body b, Sphere sphere);
void forgeBody(Orhescyon::GeneralManager& gm, Orhescyon::Entity e, Body b, Box box);
void forgeBody(Orhescyon::GeneralManager& gm, Orhescyon::Entity e, Body b, Capsule capsule);
void forgeBody(Orhescyon::GeneralManager& gm, Orhescyon::Entity e, Body b, Cylinder cylinder);
void forgeBody(Orhescyon::GeneralManager& gm, Orhescyon::Entity e, Body b, ConvexHull convexHull);
void forgeBody(Orhescyon::GeneralManager& gm, Orhescyon::Entity e, JPH::BodyID bodyID);
} // namespace Smith::Phys
