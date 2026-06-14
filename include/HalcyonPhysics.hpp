#pragma once

// Init and contexts
#include "../src/PhysicsCore/PhysicsInit.hpp"
#include "../src/PhysicsCore/PhysContexts.hpp"

// Utilities
#include "../src/PhysicsCore/JoltGlm.hpp"
#include "../src/PhysicsCore/PhysLayers.hpp"
#include "../src/PhysicsCore/PhysShapes.hpp"

// Components
#include "../src/PhysicsCore/Components/PhysBodyComponent.hpp"
#include "../src/PhysicsCore/Components/PhysManagerComponent.hpp"
#include "../src/PhysicsCore/Components/PhysTickRateComponent.hpp"
#include "../src/PhysicsCore/Components/PhysTransformSnapshotComponent.hpp"

// Managers
#include "../src/PhysicsCore/Managers/PhysManager.hpp"

// Systems
#include "../src/PhysicsCore/Systems/PhysSnapshotSystem.hpp"
#include "../src/PhysicsCore/Systems/PhysUpdateSystem.hpp"
