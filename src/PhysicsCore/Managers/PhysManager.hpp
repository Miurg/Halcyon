#pragma once

#include <Orhescyon/GeneralManager.hpp>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include "../PhysLayers.hpp"
#include <glm/ext/vector_float3.hpp>

struct SnapshotIndices
{
	uint8_t previous;
	uint8_t current;
	uint8_t writing;
};

using Orhescyon::GeneralManager;
class PhysManager
{
public:
	PhysManager(GeneralManager& gm);

	MyBroadPhaseLayerInterface sBroadPhaseLayerInterface;
	MyObjectVsBroadPhaseLayerFilter sObjectVsBroadPhaseLayerFilter;
	MyObjectLayerPairFilter sObjectLayerPairFilter;

	JPH::PhysicsSystem* physicsSystem = new JPH::PhysicsSystem();
	JPH::TempAllocatorImpl* tempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);
	JPH::JobSystemThreadPool* jobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers,
	                                                                   std::thread::hardware_concurrency() - 1);
	
	std::atomic<SnapshotIndices> physSnapshot{{0, 1, 2}};

	JPH::BodyID createDynamicSphere(glm::vec3 pos, float radius);
	JPH::BodyID createStaticBox(glm::vec3 pos, glm::vec3 halfExtents);

private:
	GeneralManager* gm;
};