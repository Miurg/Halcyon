#include "Phys.hpp"
#include "Phys.hpp"
#include <Jolt/Physics/Body/BodyID.h>
#include "../PhysicsCore/Managers/PhysManager.hpp"
#include "../PhysicsCore/PhysContexts.hpp"
#include "../PhysicsCore/PhysShapes.hpp"

static void internalForgeBody(Orhescyon::GeneralManager& gm, Orhescyon::Entity e, Body b, Shape shape)
{
	PhysManager& physManager = *gm.getContextComponent<PhysManagerContext, PhysManagerComponent>()->physManager;

	if (!gm.hasComponent<LocalTransformComponent>(e))
	{
		gm.addComponent<GlobalTransformComponent>(e, b.pos);
	}

	if (!gm.hasComponent<PhysBodyComponent>(e))
	{
		PhysBodyComponent* physBody = gm.addComponent<PhysBodyComponent>(e);
		physBody->bodyID = physManager.createBody(b, shape);
	}

	if (!gm.hasComponent<PhysTransformSnapshotComponent>(e))
	{
		gm.addComponent<PhysTransformSnapshotComponent>(e);
	}

	if (!gm.isSubscribedTo<PhysSnapshotSystem>(e))
	{
		gm.subscribeEntity<PhysSnapshotSystem>(e);
	}

	if (!gm.isSubscribedTo<PhysSyncSystem>(e))
	{
		gm.subscribeEntity<PhysSyncSystem>(e);
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
	GlobalTransformComponent* globalT = gm.getComponent<GlobalTransformComponent>(e);
	if (!globalT)
	{
		gm.addComponent<GlobalTransformComponent>(e);
	}
	LocalTransformComponent* localT = gm.getComponent<LocalTransformComponent>(e);
	if (!localT)
	{
		gm.addComponent<LocalTransformComponent>(e);
	}
	RelationshipComponent* relationship = gm.getComponent<RelationshipComponent>(e);
	if (!relationship)
	{
		gm.addComponent<RelationshipComponent>(e);
	}
	PhysBodyComponent* physBody = gm.getComponent<PhysBodyComponent>(e);
	if (!physBody)
	{
		physBody = gm.addComponent<PhysBodyComponent>(e);
		physBody->bodyID = bodyID;
	}
	PhysTransformSnapshotComponent* snapshot = gm.getComponent<PhysTransformSnapshotComponent>(e);
	if (!snapshot)
	{
		snapshot = gm.addComponent<PhysTransformSnapshotComponent>(e);
	}
	gm.subscribeEntity<PhysSnapshotSystem>(e);
	gm.subscribeEntity<TransformSystem>(e);
	gm.subscribeEntity<PhysSyncSystem>(e);
}
