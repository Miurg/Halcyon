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

namespace
{
//=== Global module-scope data ===
bool WireframeToggle = false;
bool CursorDisable = true;

GLuint ScreenWidth = 1920;
GLuint ScreenHeight = 1080;

Camera MainCamera(glm::vec3(10.0f, 10.0f, 10.0f), glm::vec3(0.0f, 1.0f, 0.0f),
                  -45.0f, // Yaw
                  -30.0f  // Pitch
);

bool keys[1024] = {false};

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

double LastMousePositionX = 400.0;
double LastMousePositionY = 300.0;

//=== Callback implementations ===
void FramebufferSizeCallback(GLFWwindow* /*window*/, int width, int height)
{
	glViewport(0, 0, width, height);
	ScreenWidth = width;
	ScreenHeight = height;
}

void KeyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mode*/)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
	if (key == GLFW_KEY_1 && action == GLFW_PRESS)
	{
		WireframeToggle = !WireframeToggle;
	}
	if (key == GLFW_KEY_3 && action == GLFW_PRESS)
	{
		if (CursorDisable)
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			CursorDisable = !CursorDisable;
		}
		else
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			CursorDisable = !CursorDisable;
		}
	}

	if (action == GLFW_PRESS)
		keys[key] = true;
	else if (action == GLFW_RELEASE)
		keys[key] = false;
}

void MouseCallback(GLFWwindow* /*window*/, double xpos, double ypos)
{
	if (CursorDisable)
	{
		GLfloat xoffset = static_cast<GLfloat>(xpos - LastMousePositionX);
		GLfloat yoffset = static_cast<GLfloat>(LastMousePositionY - ypos);
		LastMousePositionX = xpos;
		LastMousePositionY = ypos;
		constexpr GLfloat sensitivity = 0.1f;
		xoffset *= sensitivity;
		yoffset *= sensitivity;
		MainCamera.ProcessMouseMovement(xoffset, yoffset, true);
	}
}
} // anonymous namespace

int Application::Run()
{
	//=== Initialize window context ===
	Window window(ScreenWidth, ScreenHeight, "VoxelParticleSimulator");

	glfwSetCursorPosCallback(window.GetHandle(), MouseCallback);
	glfwSetCursorPos(window.GetHandle(), LastMousePositionX, LastMousePositionY);

	glfwSetFramebufferSizeCallback(window.GetHandle(), FramebufferSizeCallback);
	glfwSetInputMode(window.GetHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//=== OpenGL options ===
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	glEnable(GL_DEPTH_TEST);

	//=== Create shader ===
	Shader ourShader(RESOURCES_PATH "instanced_shader.vert", RESOURCES_PATH "instanced_shader.frag");

	//=== ECS ===
	GeneralManager world;

	world.RegisterSystem<ControlSystem>();
	world.RegisterSystem<InputSolverSystem>();
	world.RegisterSystem<MovementSystem>();
	world.RegisterSystem<RotationSystem>();
	world.RegisterSystem<CameraSystem>(keys);
	world.RegisterSystem<MultiDrawIndirectRenderingSystem>(ourShader, MainCamera, &ScreenWidth, &ScreenHeight);

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

	// Context create
	Entity windowAndInputEntity = world.CreateEntity();
	world.AddComponent<WindowComponent>(windowAndInputEntity, &window);
	world.AddComponent<KeyboardStateComponent>(windowAndInputEntity);
	world.AddComponent<MouseStateComponent>(windowAndInputEntity);
	world.AddComponent<CursorPositionComponent>(windowAndInputEntity);
	world.AddComponent<WindowSizeComponent>(windowAndInputEntity);
	world.AddComponent<ScrollDeltaComponent>(windowAndInputEntity);
	world.SubscribeEntity<InputSolverSystem>(windowAndInputEntity);

	auto inputCtx = std::make_shared<InputDataContext>();
	inputCtx->InputInstance = windowAndInputEntity;
	world.RegisterContext<InputDataContext>(inputCtx);

	auto windowCtx = std::make_shared<MainWindowContext>();
	windowCtx->WindowInstance = windowAndInputEntity;
	world.RegisterContext<MainWindowContext>(windowCtx);

	// Camera as entity
	Entity cameraEntity = world.CreateEntity();
	world.AddComponent<CameraComponent>(cameraEntity, &MainCamera);
	world.SubscribeEntity<CameraSystem>(cameraEntity);

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

	//=== Main loop ===
	int frames = 0;
	float fpsLastTime = static_cast<float>(glfwGetTime());

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