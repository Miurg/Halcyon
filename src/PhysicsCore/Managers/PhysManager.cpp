#include "PhysManager.hpp"
#include "../JoltGlm.hpp"
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>

PhysManager::PhysManager(GeneralManager& gm) : gm(&gm)
{
	physicsSystem->Init(4096, 0, 4096, 4096, sBroadPhaseLayerInterface, sObjectVsBroadPhaseLayerFilter,
	                    sObjectLayerPairFilter);
}

JPH::BodyID PhysManager::createDynamicSphere(glm::vec3 pos, float radius)
{
	JPH::BodyCreationSettings dynamicSphere(new JPH::SphereShape(radius), toJolt(pos),
	                                       JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::MOVING);

	return physicsSystem->GetBodyInterface().CreateAndAddBody(dynamicSphere, JPH::EActivation::Activate);
}

JPH::BodyID PhysManager::createStaticBox(glm::vec3 pos, glm::vec3 halfExtents)
{
	JPH::BodyCreationSettings staticBox(new JPH::BoxShape(toJolt(halfExtents)), toJolt(pos),
	                                        JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::NON_MOVING);

	return physicsSystem->GetBodyInterface().CreateAndAddBody(staticBox, JPH::EActivation::Activate);
}
