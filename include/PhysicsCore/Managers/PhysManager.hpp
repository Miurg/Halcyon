#pragma once

#include "HalcyonExport.hpp"
#include <Orhescyon/GeneralManager.hpp>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include "PhysicsCore/PhysLayers.hpp"
#include "PhysicsCore/PhysShapes.hpp"
#include <glm/ext/vector_float3.hpp>

struct HALCYON_API SnapshotIndices
{
	uint8_t previous;
	uint8_t current;
	uint8_t writing;
};

using Orhescyon::GeneralManager;
class HALCYON_API PhysManager
{
public:
	PhysManager(GeneralManager& gm);

	MyBroadPhaseLayerInterface sBroadPhaseLayerInterface;
	MyObjectVsBroadPhaseLayerFilter sObjectVsBroadPhaseLayerFilter;
	MyObjectLayerPairFilter sObjectLayerPairFilter;

	JPH::PhysicsSystem* physicsSystem = nullptr;
	JPH::TempAllocatorImpl* tempAllocator = nullptr;
	JPH::JobSystemThreadPool* jobSystem = nullptr;
	
	std::atomic<SnapshotIndices> physSnapshot{{0, 1, 2}};

	JPH::BodyID createDynamicSphere(glm::vec3 pos, float radius);
	JPH::BodyID createStaticBox(glm::vec3 pos, glm::vec3 halfExtents);

	JPH::BodyID createBody(const Body& body, const Shape& shape);

private:
	static JPH::Shape* makeJoltShape(const Shape& shape);

	GeneralManager* gm;
};