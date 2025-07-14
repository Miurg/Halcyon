#include "Application.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include "RenderCore/Shader.h"
#include "RenderCore/Camera.h"
#include "ECSCore/ECSManager.h"
#include "RenderCore/AssetManager.h"
#include "RenderCore/MeshAsset.h"
#include "RenderCore/MaterialAsset.h"
#include "RenderCore/PrimitiveMeshFactory.h"
#include "ECSCore/Systems/CameraSystem.h"
#include "ECSCore/Systems/RenderingSystem.h"
#include "ECSCore/Systems/RotationSystem.h"
#include "ECSCore/Systems/MovementSystem.h"

namespace
{
    //=== Global module-scope data ===
    bool WireframeToggle = false;
    bool CursorDisable   = true;

    GLuint ScreenWidth  = 1920;
    GLuint ScreenHeight = 1080;

    Camera MainCamera
    (
        glm::vec3(10.0f,  10.0f, 10.0f),
        glm::vec3(0.0f,    1.0f,  0.0f),
        -45.0f, // Yaw
        -30.0f  // Pitch
    );

    bool keys[1024] = { false };

    GLfloat deltaTime = 0.0f;
    GLfloat lastFrame = 0.0f;

    double LastMousePositionX = 400.0;
    double LastMousePositionY = 300.0;

    //=== Callback prototypes ===
    void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
    void MouseCallback(GLFWwindow* window, double xpos, double ypos);
    void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);

    //=== Callback implementations ===
    void FramebufferSizeCallback(GLFWwindow* /*window*/, int width, int height)
    {
        glViewport(0, 0, width, height);
        ScreenWidth  = width;
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
} //anonymous namespace

//=== Main loop implementation ===
int Application::Run()
{
    //=== Initialize GLFW and create window ===
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(ScreenWidth, ScreenHeight, "VoxelParticleSimulator", nullptr, nullptr);
    if (window == nullptr)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwSetKeyCallback(window, KeyCallback);
    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetCursorPos(window, LastMousePositionX, LastMousePositionY);

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return -1;
    }

    glViewport(0, 0, ScreenWidth, ScreenHeight);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    //=== OpenGL options ===
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);
    glEnable(GL_DEPTH_TEST);

    //=== Create shader ===
    Shader ourShader(RESOURCES_PATH "shader.vert", RESOURCES_PATH "shader.frag");

    //=== Resource manager ===
    AssetManager assetManager;

    //=== Create meshes ===
    MeshAsset* cubeMesh      = PrimitiveMeshFactory::CreateCube(&assetManager, "cube");
    MeshAsset* sphereMesh    = PrimitiveMeshFactory::CreateSphere(&assetManager, "sphere", 64, 64);
    MeshAsset* planeMesh     = PrimitiveMeshFactory::CreatePlane(&assetManager, "plane", 1.0f, 2.0f);
    MeshAsset* cylinderMesh  = PrimitiveMeshFactory::CreateCylinder(&assetManager, "cylinder", 0.5f, 1.0f, 16);
    MeshAsset* coneMesh      = PrimitiveMeshFactory::CreateCone(&assetManager, "cone", 0.5f, 1.0f, 16);

    //=== Materials ===
    MaterialAsset* cubeMaterial = assetManager.CreateMaterial("cube_material");
    cubeMaterial->AddTexture(RESOURCES_PATH "awesomeface.png", "diffuse");

    MaterialAsset* altMaterial  = assetManager.CreateMaterial("alt_material");
    altMaterial->AddTexture(RESOURCES_PATH "container.jpg", "diffuse");

    //=== ECS ===
    ECSManager world;

    world.AddSystemToECSManager<RenderingSystem>(ourShader, MainCamera, &ScreenWidth, &ScreenHeight);
    world.AddSystemToECSManager<MovementSystem>();
    world.AddSystemToECSManager<RotationSystem>();
    world.AddSystemToECSManager<CameraSystem>(keys);

    //Camera as entity
    Entity cameraEntity = world.CreateEntity();
    world.AddComponentToEntity<CameraComponent>(cameraEntity, &MainCamera);
    world.SubscribeEntityToSystem<CameraSystem>(cameraEntity);

    //=== Create entities ===
    for (GLuint i = 0; i < 100; ++i)
    {
        for (GLuint j = 0; j < 10; ++j)
        {
            for (GLuint k = 0; k < 10; ++k)
            {
                Entity entity = world.CreateEntity();
                world.AddComponentToEntity<TransformComponent>(entity, glm::vec3(i * 2.0f, k * 2.0f, j * 2.0f));

                //Mesh and material variety
                MeshAsset* currentMesh      = cubeMesh;
                MaterialAsset* currentMaterial = cubeMaterial;

                int meshType = (i + j + k) % 5;
                switch (meshType)
                {
                    case 0: currentMesh = cubeMesh;     break;
                    case 1: currentMesh = sphereMesh;   break;
                    case 2: currentMesh = planeMesh;    break;
                    case 3: currentMesh = cylinderMesh; break;
                    case 4: currentMesh = coneMesh;     break;
                }

                if ((i + j + k) % 2 == 0)
                {
                    currentMaterial = altMaterial;
                }

                world.AddComponentToEntity<RenderableComponent>(entity, currentMesh, currentMaterial);

                if ((i + j + k) % 2 == 0)
                {
                    world.AddComponentToEntity<VelocityComponent>(entity, glm::vec3(0.5f, 0.0f, 0.5f));
                    world.SubscribeEntityToSystem<MovementSystem>(entity);
                }

                if ((i + j + k) % 2 == 0)
                {
                    world.AddComponentToEntity<RotationSpeedComponent>(entity, 500.0f, glm::vec3(0.0f, 1.0f, 0.0f));
                    world.SubscribeEntityToSystem<RotationSystem>(entity);
                }

                world.SubscribeEntityToSystem<RenderingSystem>(entity);
            }
        }
    }

    //=== Main loop ===
    int   frames      = 0;
    float fpsLastTime = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        GLfloat currentFrame = static_cast<GLfloat>(glfwGetTime());
        deltaTime            = currentFrame - lastFrame;
        lastFrame            = currentFrame;

        //Rendering mode
        if (WireframeToggle)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glClearColor(0.3f, 0.3f, 1.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        world.Update(deltaTime);

        //FPS
        frames++;
        float now = static_cast<float>(glfwGetTime());
        if (now - fpsLastTime >= 1.0f)
        {
            std::cout << "FPS: " << frames << std::endl;
            frames      = 0;
            fpsLastTime = now;
        }

        glfwSwapBuffers(window);
    }

    //=== Cleanup resources ===
    assetManager.Cleanup();
    glfwTerminate();

    return 0;
} 