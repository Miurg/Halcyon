#include "PhysicsInit.hpp"
#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <thread>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include "Components/PhysManagerComponent.hpp"
#include "Components/PhysBodyComponent.hpp"
#include "PhysContexts.hpp"
#include "Systems/PhysUpdateSystem.hpp"
#include "PhysLayers.hpp"

static JPH::Factory* sFactory = nullptr;

void PhysicsInit::Run(Orhescyon::GeneralManager& gm)
{
	JPH::RegisterDefaultAllocator();
	
	if (sFactory == nullptr)
	{
		sFactory = new JPH::Factory();
		JPH::Factory::sInstance = sFactory;
	}

	JPH::RegisterTypes();
	Entity physManagerEntity = gm.createEntity();
	PhysManager* physManager = new PhysManager(gm);
	gm.addComponent<PhysManagerComponent>(physManagerEntity, physManager);
	gm.registerContext<PhysManagerContext>(physManagerEntity);
}

