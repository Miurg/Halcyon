#include "Application.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include "CoreInit.h"
#include "ContextInit.h"
#include "SceneInit.h"
#include "Core/GeneralManager.h"
#include "GLFWCore/Contexts/InputDataContext.h"
#include "GLFWCore/Contexts/MainWindowContext.h"
#include "GLFWCore/Systems/InputSolverSystem.h"
#include "GLFWCore/Window.h"
#include "RenderCore/AssetManager.h"
#include "RenderCore/Components/RenderableComponent.h"
#include "RenderCore/Components/ShaderComponent.h"
#include "RenderCore/Components/TransformComponent.h"
#include "RenderCore/Contexts/StandartShaderContext.h"
#include "RenderCore/MaterialAsset.h"
#include "RenderCore/MeshAsset.h"
#include "RenderCore/PrimitiveMeshFactory.h"
#include "RenderCore/Shader.h"
#include "RenderCore/Systems/MultiDrawIndirectRenderingSystem.h"
#include "SimulationCore/Contexts/MainCameraContext.h"
#include "SimulationCore/Systems/ControlSystem.h"
#include "SimulationCore/Systems/MovementSystem.h"
#include "SimulationCore/Systems/RotationSystem.h"

namespace
{
GLuint ScreenWidth = 1920;
GLuint ScreenHeight = 1080;

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;
} // anonymous namespace

int Application::Run()
{
	//=== ECS ===
	GeneralManager gm;

	//=== Initialize window context ===
	Window window(ScreenWidth, ScreenHeight, "VoxelParticleSimulator");
	glfwSetInputMode(window.GetHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	CoreInit::Run(gm);
	ContextInit::Run(gm, window, ScreenWidth, ScreenHeight);
	AssetManager assetManager;
	SceneInit::Run(gm, assetManager);
	
	//=== OpenGL options ===
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	glEnable(GL_DEPTH_TEST);

	//=== Main loop ===
	int frames = 0;
	float fpsLastTime = static_cast<float>(glfwGetTime());
	float i = 0;
	while (!window.ShouldClose())
	{
		glfwPollEvents();

		GLfloat currentFrame = static_cast<GLfloat>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glClearColor(0.3f, 0.3f, 1.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		gm.Update(deltaTime);
		// FPS
		frames++;
		float now = static_cast<float>(glfwGetTime());
		if (now - fpsLastTime >= 1.0f)
		{
			std::cout << "FPS: " << frames << std::endl;
			frames = 0;
			fpsLastTime = now;
		}
		window.SwapBuffers();
	}

	assetManager.Cleanup();

	return 0;
}