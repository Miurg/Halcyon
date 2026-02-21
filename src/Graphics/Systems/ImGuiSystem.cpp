#include "ImGuiSystem.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <iostream>
#include "../../Core/GeneralManager.hpp"
#include "../GraphicsContexts.hpp"
#include "../Components/GlobalTransformComponent.hpp"
#include "../Components/CameraComponent.hpp"
#include <glm/gtc/type_ptr.hpp>

void ImGuiSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "ImGuiSystem registered!" << std::endl;
}

void ImGuiSystem::onShutdown(GeneralManager& gm)
{
	std::cout << "ImGuiSystem shutdown!" << std::endl;
}

void ImGuiSystem::update(float deltaTime, GeneralManager& gm)
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::Begin("Main Camera Info");

	GlobalTransformComponent* mainCameraTransform =
	    gm.getContextComponent<MainCameraContext, GlobalTransformComponent>();
	CameraComponent* mainCamera = gm.getContextComponent<MainCameraContext, CameraComponent>();

	if (mainCameraTransform && mainCamera)
	{
		bool transformChanged = false;

		ImGui::SeparatorText("Transform");
		if (ImGui::InputFloat3("Position", glm::value_ptr(mainCameraTransform->globalPosition)))
		{
			transformChanged = true;
		}

		glm::vec3 euler = glm::degrees(glm::eulerAngles(mainCameraTransform->globalRotation));
		if (ImGui::InputFloat3("Rotation", glm::value_ptr(euler)))
		{
			mainCameraTransform->globalRotation = glm::quat(glm::radians(euler));
			transformChanged = true;
		}

		if (transformChanged)
		{
			mainCameraTransform->updateDirectionVectors();
		}

		ImGui::SeparatorText("Camera Parameters");
		ImGui::DragFloat("FOV", &mainCamera->fov, 0.1f, 1.0f, 179.0f);
		ImGui::DragFloat("Near Plane", &mainCamera->zNear, 0.01f, 0.001f, 10.0f);
		ImGui::DragFloat("Far Plane", &mainCamera->zFar, 1.0f, 10.0f, 10000.0f);
	}
	ImGui::Text("FPS: %d", fps);
	frameCount++;
	time += deltaTime;
	if (time >= 1.0f)
	{
		fps = frameCount;
		frameCount = 0;
		time = 0;
	}

	ImGui::End();
	//ImGui::ShowDemoWindow();
}
