#pragma once

#include "Core/GeneralManager.h"
#include "SimulationCore/Systems/ControlSystem.h"
#include "SimulationCore/Systems/MovementSystem.h"
#include "SimulationCore/Systems/RotationSystem.h"
#include "RenderCore/Systems/MultiDrawIndirectRenderingSystem.h"
#include "GLFWCore/Systems/InputSolverSystem.h"

#include "RenderCore/Components/TransformComponent.h"
#include "RenderCore/Components/RenderableComponent.h"
#include "RenderCore/Components/CameraComponent.h"
#include "SimulationCore/Components/RotationSpeedComponent.h"
#include "SimulationCore/Components/VelocityComponent.h"
#include "GLFWCore/Components/WindowComponent.h"
#include "GLFWCore/Components/CursorPositionComponent.h"
#include "GLFWCore/Components/KeyboardStateComponent.h"
#include "GLFWCore/Components/MouseStateComponent.h"
#include "GLFWCore/Components/ScrollDeltaComponent.h"
#include "GLFWCore/Components/WindowSizeComponent.h"
#include "RenderCore/Components/ShaderComponent.h"

class CoreInit
{
public:
	static void Run(GeneralManager& gm) 
	{
		std::cout << "COREINIT::RUN::Start init" << std::endl;

		gm.RegisterSystem<InputSolverSystem>();
		gm.RegisterSystem<ControlSystem>();
		gm.RegisterSystem<MovementSystem>();
		gm.RegisterSystem<RotationSystem>();
		gm.RegisterSystem<MultiDrawIndirectRenderingSystem>();

		gm.RegisterComponentType<CameraComponent>();
		gm.RegisterComponentType<TransformComponent>();
		gm.RegisterComponentType<RenderableComponent>();
		gm.RegisterComponentType<RotationSpeedComponent>();
		gm.RegisterComponentType<VelocityComponent>();
		gm.RegisterComponentType<WindowComponent>();
		gm.RegisterComponentType<CursorPositionComponent>();
		gm.RegisterComponentType<KeyboardStateComponent>();
		gm.RegisterComponentType<MouseStateComponent>();
		gm.RegisterComponentType<ScrollDeltaComponent>();
		gm.RegisterComponentType<WindowSizeComponent>();
		gm.RegisterComponentType<ShaderComponent>();

		std::cout << "COREINIT::RUN::Succes!" << std::endl;
	};
};