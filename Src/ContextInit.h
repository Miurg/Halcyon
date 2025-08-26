#pragma once

#include "Core/GeneralManager.h"
#include "SimulationCore/Contexts/MainCameraContext.h"
#include "RenderCore/Contexts/StandartShaderContext.h"
#include "GLFWCore/Contexts/InputDataContext.h"
#include "GLFWCore/Contexts/MainWindowContext.h"

class ContextInit
{
public:
	static void Run(GeneralManager& gm, Window& window, unsigned int ScreenWidth, unsigned int ScreenHeight)
	{
		std::cout << "CONTEXTINIT::RUN::Start init" << std::endl;
		// Window and input
		Entity windowAndInputEntity = gm.CreateEntity();
		gm.AddComponent<WindowComponent>(windowAndInputEntity, &window);
		gm.AddComponent<KeyboardStateComponent>(windowAndInputEntity);
		gm.AddComponent<MouseStateComponent>(windowAndInputEntity);
		gm.AddComponent<CursorPositionComponent>(windowAndInputEntity);
		gm.AddComponent<WindowSizeComponent>(windowAndInputEntity, ScreenWidth, ScreenHeight);
		gm.AddComponent<ScrollDeltaComponent>(windowAndInputEntity);
		gm.SubscribeEntity<InputSolverSystem>(windowAndInputEntity);
		auto inputCtx = std::make_shared<InputDataContext>();
		inputCtx->InputInstance = windowAndInputEntity;
		gm.RegisterContext<InputDataContext>(inputCtx);
		auto windowCtx = std::make_shared<MainWindowContext>();
		windowCtx->WindowInstance = windowAndInputEntity;
		gm.RegisterContext<MainWindowContext>(windowCtx);

		// Camera
		Entity cameraEntity = gm.CreateEntity();
		gm.AddComponent<CameraComponent>(cameraEntity, glm::vec3(10.0f, 10.0f, 10.0f), glm::vec3(0.0f, 1.0f, 0.0f),
		                                 -45.0f, -30.0f);
		auto cameraCtx = std::make_shared<MainCameraContext>();
		cameraCtx->CameraInstance = cameraEntity;
		gm.RegisterContext<MainCameraContext>(cameraCtx);

		// Shader
		Entity shaderEntity = gm.CreateEntity();
		gm.AddComponent<ShaderComponent>(shaderEntity, RESOURCES_PATH "instanced_shader.vert",
		                                 RESOURCES_PATH "instanced_shader.frag");
		auto shaderCtx = std::make_shared<StandartShaderContext>();
		shaderCtx->ShaderInstance = shaderEntity;
		gm.RegisterContext<StandartShaderContext>(shaderCtx);
		std::cout << "CONTEXTINIT::RUN::Succes!" << std::endl;
	};
};