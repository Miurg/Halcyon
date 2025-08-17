#include "Application.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>

#include "Core/GeneralManager.h"
#include "RenderCore/AssetManager.h"
#include "RenderCore/Camera.h"
#include "RenderCore/Components/TransformComponent.h"
#include "RenderCore/Components/RenderableComponent.h"
#include "RenderCore/MaterialAsset.h"
#include "RenderCore/MeshAsset.h"
#include "RenderCore/PrimitiveMeshFactory.h"
#include "RenderCore/Shader.h"
#include "RenderCore/Systems/MultiDrawIndirectRenderingSystem.h"
#include "SimulationCore/Systems/CameraSystem.h"
#include "SimulationCore/Systems/MovementSystem.h"
#include "SimulationCore/Systems/RotationSystem.h"
#include "GLFWCore/Window.h"
#include "GLFWCore/Systems/InputSolverSystem.h"
#include "GLFWCore/Contexts/InputDataContext.h"
#include "GLFWCore/Contexts/MainWindowContext.h"
#include "SimulationCore/Systems/ControlSystem.h"
#include "SimulationCore/Contexts/MainCameraContext.h"
#include "RenderCore/Components/ShaderComponent.h"
#include "RenderCore/Contexts/StandartShaderContext.h"

namespace
{
GLuint ScreenWidth = 1920;
GLuint ScreenHeight = 1080;

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;
} // anonymous namespace

int Application::Run()
{
	//=== Initialize window context ===
	Window window(ScreenWidth, ScreenHeight, "VoxelParticleSimulator");
	glfwSetInputMode(window.GetHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//=== ECS ===
	GeneralManager world;

	world.RegisterSystem<InputSolverSystem>();
	world.RegisterSystem<ControlSystem>();
	world.RegisterSystem<MovementSystem>();
	world.RegisterSystem<RotationSystem>();
	world.RegisterSystem<CameraSystem>();
	world.RegisterSystem<MultiDrawIndirectRenderingSystem>();

	world.RegisterComponentType<CameraComponent>();
	world.RegisterComponentType<TransformComponent>();
	world.RegisterComponentType<RenderableComponent>();
	world.RegisterComponentType<RotationSpeedComponent>();
	world.RegisterComponentType<VelocityComponent>();
	world.RegisterComponentType<WindowComponent>();
	world.RegisterComponentType<CursorPositionComponent>();
	world.RegisterComponentType<KeyboardStateComponent>();
	world.RegisterComponentType<MouseStateComponent>();
	world.RegisterComponentType<ScrollDeltaComponent>();
	world.RegisterComponentType<WindowSizeComponent>();

	// Window and input
	Entity windowAndInputEntity = world.CreateEntity();
	world.AddComponent<WindowComponent>(windowAndInputEntity, &window);
	world.AddComponent<KeyboardStateComponent>(windowAndInputEntity);
	world.AddComponent<MouseStateComponent>(windowAndInputEntity);
	world.AddComponent<CursorPositionComponent>(windowAndInputEntity);
	world.AddComponent<WindowSizeComponent>(windowAndInputEntity, ScreenWidth, ScreenHeight);
	world.AddComponent<ScrollDeltaComponent>(windowAndInputEntity);
	world.SubscribeEntity<InputSolverSystem>(windowAndInputEntity);
	auto inputCtx = std::make_shared<InputDataContext>();
	inputCtx->InputInstance = windowAndInputEntity;
	world.RegisterContext<InputDataContext>(inputCtx);
	auto windowCtx = std::make_shared<MainWindowContext>();
	windowCtx->WindowInstance = windowAndInputEntity;
	world.RegisterContext<MainWindowContext>(windowCtx);

	// Camera
	Entity cameraEntity = world.CreateEntity();
	Camera MainCamera(glm::vec3(10.0f, 10.0f, 10.0f), glm::vec3(0.0f, 1.0f, 0.0f),
	                  -45.0f, // Yaw
	                  -30.0f  // Pitch
	);
	world.AddComponent<CameraComponent>(cameraEntity, &MainCamera);
	world.SubscribeEntity<CameraSystem>(cameraEntity);
	auto cameraCtx = std::make_shared<MainCameraContext>();
	cameraCtx->CameraInstance = cameraEntity;
	world.RegisterContext<MainCameraContext>(cameraCtx);

	// Shader
	Entity shaderEntity = world.CreateEntity();
	Shader mainShader(RESOURCES_PATH "instanced_shader.vert", RESOURCES_PATH "instanced_shader.frag");
	world.AddComponent<ShaderComponent>(shaderEntity, &mainShader);
	auto shaderCtx = std::make_shared<StandartShaderContext>();
	shaderCtx->ShaderInstance = shaderEntity;
	world.RegisterContext<StandartShaderContext>(shaderCtx);

	//=== Resource manager ===
	AssetManager assetManager;

	//=== Create meshes ===
	MeshAsset* cubeMesh = PrimitiveMeshFactory::CreateCube(&assetManager, "cube");
	MeshAsset* sphereMesh = PrimitiveMeshFactory::CreateSphere(&assetManager, "sphere", 16, 16);
	MeshAsset* planeMesh = PrimitiveMeshFactory::CreatePlane(&assetManager, "plane", 1.0f, 2.0f);
	MeshAsset* cylinderMesh = PrimitiveMeshFactory::CreateCylinder(&assetManager, "cylinder", 0.5f, 1.0f, 16);
	MeshAsset* coneMesh = PrimitiveMeshFactory::CreateCone(&assetManager, "cone", 0.5f, 1.0f, 16);

	//=== Materials ===
	MaterialAsset* cubeMaterial = assetManager.CreateMaterial("cube_material");
	cubeMaterial->AddTexture(RESOURCES_PATH "awesomeface.png", "diffuse");

	MaterialAsset* altMaterial = assetManager.CreateMaterial("alt_material");
	altMaterial->AddTexture(RESOURCES_PATH "container.jpg", "diffuse");

	//=== Create entities ===
	for (int i = 0; i < 100; ++i)
	{
		for (int j = 0; j < 100; ++j)
		{
			for (int k = 0; k < 100; ++k)
			{
				Entity entity = world.CreateEntity();
				world.AddComponent<TransformComponent>(entity, glm::vec3(i * 2.0f, k * 2.0f, j * 2.0f));

				// Mesh and material variety
				MeshAsset* currentMesh = cubeMesh;
				MaterialAsset* currentMaterial = cubeMaterial;

				int meshType = (i + j + k) % 5;
				switch (meshType)
				{
				case 0:
					currentMesh = cubeMesh;
					break;
				case 1:
					currentMesh = sphereMesh;
					break;
				case 2:
					currentMesh = planeMesh;
					break;
				case 3:
					currentMesh = cylinderMesh;
					break;
				case 4:
					currentMesh = coneMesh;
					break;
				}

				if ((i + j + k) % 2 == 0)
				{
					currentMaterial = altMaterial;
				}

				world.AddComponent<RenderableComponent>(entity, currentMesh, currentMaterial);

				if ((i + j + k) % 2 == 0)
				{
					world.AddComponent<RotationSpeedComponent>(entity, 500.0f, glm::vec3(0.0f, 1.0f, 0.0f));
					world.SubscribeEntity<RotationSystem>(entity);
					world.AddComponent<VelocityComponent>(entity, glm::vec3(0.5f, 0.0f, 0.5f));
					world.SubscribeEntity<MovementSystem>(entity);
				}

				world.SubscribeEntity<MultiDrawIndirectRenderingSystem>(entity);
			}
		}
	}
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

		world.Update(deltaTime);
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

	//=== Cleanup resources ===
	assetManager.Cleanup();

	return 0;
}