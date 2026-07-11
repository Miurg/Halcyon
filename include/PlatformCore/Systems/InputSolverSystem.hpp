#pragma once

#include "HalcyonExport.hpp"
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include "PlatformCore/Components/CursorPositionComponent.hpp"
#include "PlatformCore/Components/WindowSizeComponent.hpp"
#include "PlatformCore/Components/KeyboardStateComponent.hpp"
#include "PlatformCore/Components/MouseStateComponent.hpp"
#include "PlatformCore/Components/ScrollDeltaComponent.hpp"
#include "PlatformCore/Components/WindowComponent.hpp"

using Orhescyon::GeneralManager;
class HALCYON_API InputSolverSystem
    : public Orhescyon::SystemCore<InputSolverSystem, WindowComponent, CursorPositionComponent, WindowSizeComponent,
                        KeyboardStateComponent, MouseStateComponent, ScrollDeltaComponent>
{
public:
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
};