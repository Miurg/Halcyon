#pragma once

// Init and contexts
#include "../src/PhysicsCore/PhysicsInit.hpp"
#include "PhysicsCore/PhysContexts.hpp"

// Utilities
#include "../src/PhysicsCore/JoltGlm.hpp"
#include "../src/PhysicsCore/PhysLayers.hpp"
#include "../src/PhysicsCore/PhysShapes.hpp"

// Components
#include "PhysicsCore/Components/PhysBodyComponent.hpp"
#include "PhysicsCore/Components/PhysManagerComponent.hpp"
#include "PhysicsCore/Components/PhysTickRateComponent.hpp"
#include "PhysicsCore/Components/PhysTransformSnapshotComponent.hpp"

// Managers
#include "../src/PhysicsCore/Managers/PhysManager.hpp"

// Systems
#include "../src/PhysicsCore/Systems/PhysSnapshotSystem.hpp"
#include "../src/PhysicsCore/Systems/PhysUpdateSystem.hpp"
