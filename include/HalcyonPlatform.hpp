#pragma once

// Init and contexts
#include "../src/PlatformCore/PlatformInit.hpp"
#include "PlatformCore/PlatformContexts.hpp"

// Core
#include "../src/PlatformCore/Window.hpp"
#include "../src/PlatformCore/InputEvent.hpp"

// Components
#include "PlatformCore/Components/CursorPositionComponent.hpp"
#include "PlatformCore/Components/KeyboardStateComponent.hpp"
#include "PlatformCore/Components/MouseStateComponent.hpp"
#include "PlatformCore/Components/ScrollDeltaComponent.hpp"
#include "PlatformCore/Components/WindowComponent.hpp"
#include "PlatformCore/Components/WindowSizeComponent.hpp"

// Systems
#include "../src/PlatformCore/Systems/InputSolverSystem.hpp"
