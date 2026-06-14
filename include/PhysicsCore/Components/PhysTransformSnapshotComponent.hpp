#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Math/Quat.h>

const int maxSnapshots = 3;

struct PhysTransformSnapshotComponent
{
	JPH::Vec3 positionSnap[maxSnapshots] = {JPH::Vec3::sZero(), JPH::Vec3::sZero(), JPH::Vec3::sZero()};
	JPH::Quat rotationSnap[maxSnapshots] = {JPH::Quat::sIdentity(), JPH::Quat::sIdentity(), JPH::Quat::sIdentity()};
};