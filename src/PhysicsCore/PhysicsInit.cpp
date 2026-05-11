#include "PhysicsInit.hpp"
#include <iostream>
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
#include "Components/PhysTickRateComponent.hpp"
#include "PhysContexts.hpp"
#include "Systems/PhysUpdateSystem.hpp"
#include "Systems/PhysSnapshotSystem.hpp"
#include "PhysLayers.hpp"

static JPH::Factory* sFactory = nullptr;

#pragma region Run
void PhysicsInit::Run(Orhescyon::GeneralManager& gm)
{
#ifdef _DEBUG
	std::cout << "PHYSICSINIT::RUN::Start init" << std::endl;
#endif //_DEBUG

	coreInit(gm);
	initPhysics(gm);

#ifdef _DEBUG
	std::cout << "PHYSICSINIT::RUN::Succes!" << std::endl;
#endif //_DEBUG
}
#pragma endregion

#pragma region coreInit
void PhysicsInit::coreInit(Orhescyon::GeneralManager& gm)
{
	gm.registerSystemManager("physics");

	gm.registerSystem<PhysUpdateSystem>();
	gm.registerSystem<PhysSnapshotSystem>();
}
#pragma endregion

#pragma region initPhysics
void PhysicsInit::initPhysics(Orhescyon::GeneralManager& gm)
{
	JPH::RegisterDefaultAllocator();

	if (sFactory == nullptr)
	{
		sFactory = new JPH::Factory();
		JPH::Factory::sInstance = sFactory;
	}

	JPH::RegisterTypes();
	Orhescyon::Entity physManagerEntity = gm.createEntity();
	PhysManager* physManager = new PhysManager(gm);
	gm.addComponent<PhysManagerComponent>(physManagerEntity, physManager);
	gm.registerContext<PhysManagerContext>(physManagerEntity);

	Orhescyon::Entity tickRateEntity = gm.createEntity();
	gm.addComponent<PhysTickRateComponent>(tickRateEntity);
	gm.registerContext<PhysTickRateContext>(tickRateEntity);
}
#pragma endregion
