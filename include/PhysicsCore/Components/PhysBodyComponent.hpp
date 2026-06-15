#pragma once

#include "HalcyonExport.hpp"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

struct HALCYON_API PhysBodyComponent
{
	JPH::BodyID bodyID;
};