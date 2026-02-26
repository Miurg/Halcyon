#include "ImGuiSystem.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <iostream>
#include "../../Core/GeneralManager.hpp"
#include "../GraphicsContexts.hpp"
#include "../Components/GlobalTransformComponent.hpp"
#include "../Components/CameraComponent.hpp"
#include "../Components/LightComponent.hpp"
#include "../Components/LocalTransformComponent.hpp"
#include "../Components/RelationshipComponent.hpp"
#include "../../Game/Components/ControlComponent.hpp"
#include "../Components/NameComponent.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <functional>
#include "../Components/CurrentFrameComponent.hpp"
#include "../../Platform/PlatformContexts.hpp"
#include "../../Platform/Components/DeltaTimeComponent.hpp"
#include "../Components/SsaoSettingsComponent.hpp"

void ImGuiSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "ImGuiSystem registered!" << std::endl;
}

void ImGuiSystem::onShutdown(GeneralManager& gm)
{
	std::cout << "ImGuiSystem shutdown!" << std::endl;
}

void ImGuiSystem::drawEntityNode(Entity entity, GeneralManager& gm)
{
	std::string entityName = "Entity " + std::to_string(entity);
	auto* nameComp = gm.getComponent<NameComponent>(entity);
	if (nameComp)
	{
		entityName = nameComp->name;
	}

	auto* relComp = gm.getComponent<RelationshipComponent>(entity);
	bool isLeaf = (relComp == nullptr || relComp->firstChild == NULL_ENTITY);

	ImGuiTreeNodeFlags flags =
	    ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (isLeaf)
	{
		flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}
	if (selectedEntity == entity)
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}

	bool nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)entity, flags, "%s", entityName.c_str());
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
	{
		selectedEntity = entity;
	}

	if (nodeOpen)
	{
		if (!isLeaf)
		{
			Entity child = relComp->firstChild;
			while (child != NULL_ENTITY)
			{
				if (gm.isActive(child))
				{
					drawEntityNode(child, gm);
				}
				auto* childRel = gm.getComponent<RelationshipComponent>(child);
				if (childRel)
				{
					child = childRel->nextSibling;
				}
				else
				{
					child = NULL_ENTITY;
				}
			}
			ImGui::TreePop();
		}
	}
}

void ImGuiSystem::update(GeneralManager& gm)
{
	CurrentFrameComponent* currentFrameComp = gm.getContextComponent<CurrentFrameContext, CurrentFrameComponent>();
	if (!currentFrameComp || !currentFrameComp->frameValid)
	{
		return;
	}
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
		ImGui::DragFloat("Ortho size", &mainCamera->orthoSize, 0.1f, 0.1f, 100.0f);
	}
	float deltaTime = gm.getContextComponent<DeltaTimeContext, DeltaTimeComponent>()->deltaTime;

	ImGui::Text("FPS: %d", fps);
	ImGui::Text("Frame time: %.5f", deltaTime);
	frameCount++;
	time += deltaTime;
	if (time >= 1.0f)
	{
		fps = frameCount;
		frameCount = 0;
		time = 0;
	}

	ImGui::End();

	// Entity Inspector Window
	ImGui::Begin("Entity Inspector");

	// Left Pane: Entity List
	ImGui::BeginChild("EntityList", ImVec2(350, 0), true);
	ImGui::Text("Active Entities");
	ImGui::Separator();

	const auto& activeEntities = gm.getActiveEntities();
	for (Entity entity : activeEntities)
	{
		auto* relComp = gm.getComponent<RelationshipComponent>(entity);
		// Top-level entities are those with no parent or no relationship component
		if (relComp == nullptr || relComp->parent == NULL_ENTITY)
		{
			drawEntityNode(entity, gm);
		}
	}
	ImGui::EndChild();

	ImGui::SameLine();

	// Right Pane: Component Details
	ImGui::BeginChild("ComponentDetails", ImVec2(0, 0), true);
	if (selectedEntity != static_cast<Entity>(-1) && gm.isActive(selectedEntity))
	{
		ImGui::Text("Inspecting Entity %u", selectedEntity);
		auto* nameComp = gm.getComponent<NameComponent>(selectedEntity);
		if (nameComp)
		{
			ImGui::SameLine();
			ImGui::InputText("##NameEdit", nameComp->name, sizeof(nameComp->name));
		}

		ImGui::Separator();

		// Global Transform Component
		if (auto* transform = gm.getComponent<GlobalTransformComponent>(selectedEntity))
		{
			if (ImGui::CollapsingHeader("Global Transform", ImGuiTreeNodeFlags_DefaultOpen))
			{
				bool transformChanged = false;

				if (ImGui::DragFloat3("G Position", glm::value_ptr(transform->globalPosition), 0.1f))
				{
					transformChanged = true;
				}

				glm::vec3 euler = glm::degrees(glm::eulerAngles(transform->globalRotation));
				if (ImGui::DragFloat3("G Rotation", glm::value_ptr(euler), 0.5f))
				{
					transform->globalRotation = glm::quat(glm::radians(euler));
					transformChanged = true;
				}

				if (ImGui::DragFloat3("G Scale", glm::value_ptr(transform->globalScale), 0.1f))
				{
					transformChanged = true;
				}

				if (transformChanged)
				{
					transform->updateDirectionVectors();
					transform->isModelDirty = true;
					transform->isViewDirty = true;
				}
			}
		}

		// Local Transform Component
		if (auto* localTransform = gm.getComponent<LocalTransformComponent>(selectedEntity))
		{
			if (ImGui::CollapsingHeader("Local Transform", ImGuiTreeNodeFlags_DefaultOpen))
			{
				bool localTransformChanged = false;

				if (ImGui::DragFloat3("L Position", glm::value_ptr(localTransform->localPosition), 0.1f))
				{
					localTransformChanged = true;
				}

				glm::vec3 localEuler = glm::degrees(glm::eulerAngles(localTransform->localRotation));
				if (ImGui::DragFloat3("L Rotation", glm::value_ptr(localEuler), 0.5f))
				{
					localTransform->localRotation = glm::quat(glm::radians(localEuler));
					localTransformChanged = true;
				}

				if (ImGui::DragFloat3("L Scale", glm::value_ptr(localTransform->localScale), 0.1f))
				{
					localTransformChanged = true;
				}

				if (localTransformChanged)
				{
					localTransform->updateDirectionVectors();
					localTransform->isModelDirty = true;
					localTransform->isViewDirty = true;
				}
			}
		}

		// Light Component
		if (auto* light = gm.getComponent<LightComponent>(selectedEntity))
		{
			if (ImGui::CollapsingHeader("Light Component", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::SeparatorText("Sun Lighting");
				ImGui::ColorEdit3("Color", glm::value_ptr(light->color));
				ImGui::DragFloat("Intensity", &light->color.w, 0.01f, 0.0f, 1000.0f);

				ImGui::SeparatorText("Ambient Lighting");
				ImGui::ColorEdit3("Ambient Color", glm::value_ptr(light->ambient));
				ImGui::DragFloat("Ambient Intensity", &light->ambient.w, 0.001f, 0.0f, 10.0f);

				ImGui::SeparatorText("Shadow Settings");
				ImGui::DragFloat("Shadow Distance", &light->shadowDistance, 1.0f);
				ImGui::DragFloat("Shadow Caster Range", &light->shadowCasterRange, 10.0f);
			}
		}

		// Camera Component
		if (auto* camera = gm.getComponent<CameraComponent>(selectedEntity))
		{
			if (ImGui::CollapsingHeader("Camera Component", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::DragFloat("FOV", &camera->fov, 0.1f, 1.0f, 179.0f);
				ImGui::DragFloat("Near Plane", &camera->zNear, 0.01f, 0.001f, 10.0f);
				ImGui::DragFloat("Far Plane", &camera->zFar, 1.0f, 10.0f, 10000.0f);
				ImGui::DragFloat("Ortho size", &camera->orthoSize, 0.1f, 0.1f, 100.0f);
			}
		}

		// Control Component
		if (auto* control = gm.getComponent<ControlComponent>(selectedEntity))
		{
			if (ImGui::CollapsingHeader("Control Component", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::DragFloat("Movement Speed", &control->movementSpeed, 0.1f, 0.0f, 100.0f);
				ImGui::DragFloat("Mouse Sensitivity", &control->mouseSensitivity, 0.01f, 0.0f, 5.0f);
			}
		}

		// SSAO Settings Component
		if (auto* ssao = gm.getComponent<SsaoSettingsComponent>(selectedEntity))
		{
			if (ImGui::CollapsingHeader("SSAO Settings", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::SliderInt("Kernel Size", &ssao->kernelSize, 1, 32);
				ImGui::DragFloat("Radius", &ssao->radius, 0.1f, 0.1f, 50.0f);
				ImGui::DragFloat("Bias", &ssao->bias, 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat("Power", &ssao->power, 0.1f, 0.1f, 20.0f);
				ImGui::SliderInt("Num Directions", &ssao->numDirections, 1, 16);
				ImGui::DragFloat("Max Screen Radius", &ssao->maxScreenRadius, 0.01f, 0.01f, 1.0f);
			}
		}
	}
	else
	{
		ImGui::Text("Select an entity to inspect.");
	}
	ImGui::EndChild();

	ImGui::End();

	// ImGui::ShowDemoWindow();
}
