#include "PhysManager.hpp"
#include "../JoltGlm.hpp"
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <stdexcept>

PhysManager::PhysManager(GeneralManager& gm) : gm(&gm)
{
	physicsSystem->Init(4096, 0, 4096, 4096, sBroadPhaseLayerInterface, sObjectVsBroadPhaseLayerFilter,
	                    sObjectLayerPairFilter);
}

JPH::BodyID PhysManager::createDynamicSphere(glm::vec3 pos, float radius)
{
	JPH::BodyCreationSettings dynamicSphere(new JPH::SphereShape(radius), toJolt(pos), JPH::Quat::sIdentity(),
	                                        JPH::EMotionType::Dynamic, Layers::MOVING);

	return physicsSystem->GetBodyInterface().CreateAndAddBody(dynamicSphere, JPH::EActivation::Activate);
}

JPH::BodyID PhysManager::createStaticBox(glm::vec3 pos, glm::vec3 halfExtents)
{
	JPH::BodyCreationSettings staticBox(new JPH::BoxShape(toJolt(halfExtents)), toJolt(pos), JPH::Quat::sIdentity(),
	                                    JPH::EMotionType::Static, Layers::NON_MOVING);

	return physicsSystem->GetBodyInterface().CreateAndAddBody(staticBox, JPH::EActivation::Activate);
}

JPH::BodyID PhysManager::createBody(const Body& body, const Shape& shape)
{
	JPH::Shape* jShape = makeJoltShape(shape);

	JPH::EMotionType motion = JPH::EMotionType::Dynamic;
	switch (body.motion)
	{
	case Motion::Static:
		motion = JPH::EMotionType::Static;
		break;
	case Motion::Kinematic:
		motion = JPH::EMotionType::Kinematic;
		break;
	case Motion::Dynamic:
		motion = JPH::EMotionType::Dynamic;
		break;
	}

	JPH::BodyCreationSettings settings(jShape, toJolt(body.pos), toJolt(body.rot), motion, body.layer);
	settings.mFriction = body.friction;
	settings.mRestitution = body.restitution;

	JPH::EActivation activation =
	    (body.motion == Motion::Static) ? JPH::EActivation::DontActivate : JPH::EActivation::Activate;

	return physicsSystem->GetBodyInterface().CreateAndAddBody(settings, activation);
}

JPH::Shape* PhysManager::makeJoltShape(const Shape& shape)
{
	return std::visit(
	    [](const auto& s) -> JPH::Shape*
	    {
		    using T = std::decay_t<decltype(s)>;
		    if constexpr (std::is_same_v<T, Sphere>)
			    return new JPH::SphereShape(s.radius);
		    else if constexpr (std::is_same_v<T, Box>)
			    return new JPH::BoxShape(toJolt(s.halfExtents));
		    else if constexpr (std::is_same_v<T, Capsule>)
			    return new JPH::CapsuleShape(s.halfHeight, s.radius);
		    else if constexpr (std::is_same_v<T, Cylinder>)
			    return new JPH::CylinderShape(s.halfHeight, s.radius);
		    else if constexpr (std::is_same_v<T, ConvexHull>)
		    {
			    JPH::Array<JPH::Vec3> pts;
			    pts.reserve(s.points.size());
			    for (const glm::vec3& p : s.points) pts.push_back(toJolt(p));

			    JPH::ConvexHullShapeSettings settings(pts);
			    JPH::ShapeSettings::ShapeResult result = settings.Create();
			    if (result.HasError())
				    throw std::runtime_error(std::string("ConvexHull build failed: ") + result.GetError().c_str());
			    return result.Get();
		    }
	    },
	    shape);
}