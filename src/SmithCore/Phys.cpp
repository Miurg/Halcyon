#include "SmithCore/Phys.hpp"

#include <Jolt/Physics/Body/BodyID.h>
#include "PhysicsCore/Managers/PhysManager.hpp"
#include "PhysicsCore/PhysContexts.hpp"
#include "PhysicsCore/PhysShapes.hpp"
#include "GraphicsCore/Components/LocalTransformComponent.hpp"

static void internalForgeBody(Orhescyon::GeneralManager& gm, Orhescyon::Entity e, Body b, Shape shape)
{
	PhysManager& physManager = *gm.getContextComponent<PhysManagerContext, PhysManagerComponent>()->physManager;

	if (!gm.hasComponent<GlobalTransformComponent>(e))
	{
		gm.addComponentImmediate<LocalTransformComponent>(e);
	}

	if (!gm.hasComponent<LocalTransformComponent>(e))
	{
		gm.addComponentImmediate<GlobalTransformComponent>(e, b.pos);
	}

	if (!gm.hasComponent<PhysBodyComponent>(e))
	{
		PhysBodyComponent* physBody = gm.addComponentImmediate<PhysBodyComponent>(e);
		physBody->bodyID = physManager.createBody(b, shape);
	}

	if (!gm.hasComponent<PhysTransformSnapshotComponent>(e))
	{
		gm.addComponentImmediate<PhysTransformSnapshotComponent>(e);
	}

	if (!gm.isSubscribedTo<PhysSnapshotSystem>(e))
	{
		gm.subscribeEntityImmediate<PhysSnapshotSystem>(e);
	}

	if (!gm.isSubscribedTo<PhysSyncSystem>(e))
	{
		gm.subscribeEntityImmediate<PhysSyncSystem>(e);
	}
}

void Smith::Phys::forgeBody(Orhescyon::GeneralManager& gm, Orhescyon::Entity e, Body b, Sphere sphere)
{
	Shape shape = sphere;
	internalForgeBody(gm, e, b, shape);
}

void Smith::Phys::forgeBody(Orhescyon::GeneralManager& gm, Orhescyon::Entity e, Body b, Box box)
{
	Shape shape = box;
	internalForgeBody(gm, e, b, shape);
}

void Smith::Phys::forgeBody(Orhescyon::GeneralManager& gm, Orhescyon::Entity e, Body b, Capsule capsule)
{
	Shape shape = capsule;
	internalForgeBody(gm, e, b, shape);
}

void Smith::Phys::forgeBody(Orhescyon::GeneralManager& gm, Orhescyon::Entity e, Body b, Cylinder cylinder)
{
	Shape shape = cylinder;
	internalForgeBody(gm, e, b, shape);
}

void Smith::Phys::forgeBody(Orhescyon::GeneralManager& gm, Orhescyon::Entity e, Body b, ConvexHull convexHull)
{
	Shape shape = convexHull;
	internalForgeBody(gm, e, b, shape);
}

void Smith::Phys::forgeBody(Orhescyon::GeneralManager& gm, Orhescyon::Entity e, JPH::BodyID bodyID)
{
	PhysManager& physManager = *gm.getContextComponent<PhysManagerContext, PhysManagerComponent>()->physManager;
	if (!gm.hasComponent<GlobalTransformComponent>(e))
	{
		gm.addComponentImmediate<GlobalTransformComponent>(e);
	}

	if (!gm.hasComponent<LocalTransformComponent>(e))
	{
		gm.addComponentImmediate<LocalTransformComponent>(e);
	}

	if (!gm.hasComponent<PhysBodyComponent>(e))
	{
		PhysBodyComponent* physBody = gm.addComponentImmediate<PhysBodyComponent>(e);
		physBody->bodyID = bodyID;
	}

	PhysTransformSnapshotComponent* snapshot = gm.getComponent<PhysTransformSnapshotComponent>(e);
	if (!snapshot)
	{
		snapshot = gm.addComponentImmediate<PhysTransformSnapshotComponent>(e);
	}

	gm.subscribeEntityImmediate<PhysSnapshotSystem>(e);
	gm.subscribeEntityImmediate<PhysSyncSystem>(e);
}
