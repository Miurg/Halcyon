#include "ImGuiSystem.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <iostream>
#include "../../Core/GeneralManager.hpp"
#include "../GraphicsContexts.hpp"
#include "../Components/GlobalTransformComponent.hpp"

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
	if (mainCameraTransform)
	{
		ImGui::Text("Position: %.2f, %.2f, %.2f", mainCameraTransform->globalPosition.x,
		            mainCameraTransform->globalPosition.y, mainCameraTransform->globalPosition.z);

		ImGui::Text("Rotation (Quat): [%.2f, %.2f, %.2f, %.2f]", mainCameraTransform->globalRotation.x,
		            mainCameraTransform->globalRotation.y, mainCameraTransform->globalRotation.z,
		            mainCameraTransform->globalRotation.w);

		glm::vec3 euler = glm::degrees(glm::eulerAngles(mainCameraTransform->globalRotation));
		ImGui::Text("Rotation (Euler): [%.2f, %.2f, %.2f]", euler.x, euler.y, euler.z);
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
	ImGui::ShowDemoWindow();
}
