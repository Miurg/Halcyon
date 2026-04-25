#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>

namespace Layers
{
static constexpr JPH::ObjectLayer NON_MOVING = 0;
static constexpr JPH::ObjectLayer MOVING = 1;
static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
} // namespace Layers

class MyBroadPhaseLayerInterface final : public JPH::BroadPhaseLayerInterface
{
public:
	virtual JPH::uint GetNumBroadPhaseLayers() const override
	{
		return 2;
	}

	virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
	{
		switch (inLayer)
		{
		case Layers::NON_MOVING:
			return JPH::BroadPhaseLayer(0);
		case Layers::MOVING:
			return JPH::BroadPhaseLayer(1);
		default:
			JPH_ASSERT(false);
			return JPH::BroadPhaseLayer(0);
		}
	}
};

class MyObjectVsBroadPhaseLayerFilter final : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
	virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
	{
		if (inLayer1 == Layers::MOVING) return true;

		if (inLayer1 == Layers::NON_MOVING) return inLayer2 == JPH::BroadPhaseLayer(1);

		return false;
	}
};

class MyObjectLayerPairFilter final : public JPH::ObjectLayerPairFilter
{
public:
	virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::ObjectLayer inLayer2) const override
	{
		if (inLayer1 == Layers::NON_MOVING && inLayer2 == Layers::NON_MOVING) return false;

		return true;
	}
};
