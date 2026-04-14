#include "ImGuiSystem.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <iostream>
#include <numeric>
#include <algorithm>
#include "../GraphicsContexts.hpp"
#include "../Components/GlobalTransformComponent.hpp"
#include "../Components/CameraComponent.hpp"
#include "../Components/DirectLightComponent.hpp"
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
#include "../Components/GraphicsSettingsComponent.hpp"
#include "../Components/VulkanDeviceComponent.hpp"
#include "../VulkanDevice.hpp"
#include "../Components/PointLightComponent.hpp"
#include "../ShaderReloader.hpp"
#include "../Components/ShaderReloaderComponent.hpp"
#include "../Components/PipelineManagerComponent.hpp"
#include "../Components/GraphicsSettingsComponent.hpp"
#include "../Components/LightProbeGridComponent.hpp"

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
		if (auto* gs = gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>())
			gs->selectedEntity = entity;
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
		ImGui::SeparatorText("Transform");
		glm::vec3 camPos   = mainCameraTransform->getGlobalPosition();
		glm::vec3 camEuler = glm::degrees(glm::eulerAngles(mainCameraTransform->getGlobalRotation()));
		bool camChanged = false;
		if (ImGui::InputFloat3("Position", glm::value_ptr(camPos)))   camChanged = true;
		if (ImGui::InputFloat3("Rotation", glm::value_ptr(camEuler))) camChanged = true;
		if (camChanged)
		{
			mainCameraTransform->setGlobalPosition(camPos);
			mainCameraTransform->setGlobalRotation(glm::quat(glm::radians(camEuler)));
		}

		ImGui::SeparatorText("Camera Parameters");
		ImGui::DragFloat("FOV", &mainCamera->fov, 0.1f, 1.0f, 179.0f);
		ImGui::DragFloat("Near Plane", &mainCamera->zNear, 0.01f, 0.001f, 10.0f);
		ImGui::DragFloat("Far Plane", &mainCamera->zFar, 1.0f, 10.0f, 10000.0f);
		ImGui::DragFloat("Ortho size", &mainCamera->orthoSize, 0.1f, 0.1f, 100.0f);
	}
	float deltaTime = gm.getContextComponent<DeltaTimeContext, DeltaTimeComponent>()->deltaTime;

	ImGui::Text("FPS: %d", fps);
	ImGui::Text("Avg Frame time: %.2f ms", avgFrameTime * 1000.0f);
	ImGui::Text("1%% Low: %.2f ms", onePercentLowFrameTime * 1000.0f);
	frameCount++;
	time += deltaTime;
	frameTimes.push_back(deltaTime);

	if (time >= 1.0f)
	{
		fps = frameCount;

		if (!frameTimes.empty())
		{
			float sum = 0.0f;
			for (float ft : frameTimes) sum += ft;
			avgFrameTime = sum / frameTimes.size();

			std::vector<float> sortedTimes = frameTimes;
			std::sort(sortedTimes.begin(), sortedTimes.end(), std::greater<float>());

			size_t onePercentCount = std::max<size_t>(1, frameTimes.size() / 100);
			float worstSum = 0.0f;
			for (size_t i = 0; i < onePercentCount; ++i)
			{
				worstSum += sortedTimes[i];
			}
			onePercentLowFrameTime = worstSum / onePercentCount;
		}

		frameTimes.clear();
		frameCount = 0;
		time = 0;
	}

	ImGui::Checkbox("Enable shader auto reload", &autoShaderReload);
	if (autoShaderReload)
	{
		ShaderReloader* sr = gm.getContextComponent<ShaderReloaderContext, ShaderReloaderComponent>()->shaderReloader;
		PipelineManager* pManager =
		    gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
		VulkanDevice* vulkanDevice =
		    gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
		sr->update(*pManager, *vulkanDevice);
	}

	if (auto* gs = gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>())
		ImGui::Checkbox("Draw AABB boxes on top", &gs->aabbAlwaysOnTop);

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
				glm::vec3 gPos   = transform->getGlobalPosition();
				glm::vec3 gEuler = glm::degrees(glm::eulerAngles(transform->getGlobalRotation()));
				glm::vec3 gScale = transform->getGlobalScale();
				bool gChanged = false;
				if (ImGui::DragFloat3("G Position", glm::value_ptr(gPos),   0.1f)) gChanged = true;
				if (ImGui::DragFloat3("G Rotation", glm::value_ptr(gEuler), 0.5f)) gChanged = true;
				if (ImGui::DragFloat3("G Scale",    glm::value_ptr(gScale), 0.1f)) gChanged = true;
				if (gChanged)
				{
					transform->setGlobalPosition(gPos);
					transform->setGlobalRotation(glm::quat(glm::radians(gEuler)));
					transform->setGlobalScale(gScale);
				}
			}
		}

		// Local Transform Component
		if (auto* localTransform = gm.getComponent<LocalTransformComponent>(selectedEntity))
		{
			if (ImGui::CollapsingHeader("Local Transform", ImGuiTreeNodeFlags_DefaultOpen))
			{
				glm::vec3 lPos   = localTransform->getLocalPosition();
				glm::vec3 lEuler = glm::degrees(glm::eulerAngles(localTransform->getLocalRotation()));
				glm::vec3 lScale = localTransform->getLocalScale();
				bool lChanged = false;
				if (ImGui::DragFloat3("L Position", glm::value_ptr(lPos),   0.1f)) lChanged = true;
				if (ImGui::DragFloat3("L Rotation", glm::value_ptr(lEuler), 0.5f)) lChanged = true;
				if (ImGui::DragFloat3("L Scale",    glm::value_ptr(lScale), 0.1f)) lChanged = true;
				if (lChanged)
				{
					localTransform->setLocalPosition(lPos);
					localTransform->setLocalRotation(glm::quat(glm::radians(lEuler)));
					localTransform->setLocalScale(lScale);
				}
			}
		}

		// Light Component
		if (auto* light = gm.getComponent<DirectLightComponent>(selectedEntity))
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
				ImGui::DragFloat("Fade start", &ssao->fadeStart, 10.0f, 1000.0f);
				ImGui::DragFloat("Fade end", &ssao->fadeEnd, 10.0f, 1000.0f);
			}
		}

		// Graphics Settings Component
		if (auto* settings = gm.getComponent<GraphicsSettingsComponent>(selectedEntity))
		{
			if (ImGui::CollapsingHeader("Graphics Settings", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Checkbox("Enable SSAO", &settings->enableSsao);
				ImGui::Checkbox("Enable FXAA", &settings->enableFxaa);
				ImGui::Checkbox("Enable Bloom", &settings->enableBloom);
				ImGui::Checkbox("Enable Vignette", &settings->enableVignette);
				if (settings->enableBloom)
				{
					ImGui::DragFloat("Bloom Threshold", &settings->bloomThreshold, 0.1f, 0.0f, 10.0f);
					ImGui::DragFloat("Bloom Knee", &settings->bloomKnee, 0.01f, 0.0f, 1.0f);
					ImGui::DragFloat("Bloom Intensity", &settings->bloomIntensity, 0.001f, 0.0f, 1.0f);
				}

				auto* vulkanDeviceComponent = gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>();
				if (vulkanDeviceComponent && vulkanDeviceComponent->vulkanDeviceInstance)
				{
					vk::SampleCountFlagBits maxSamples = vulkanDeviceComponent->vulkanDeviceInstance->maxMsaaSamples;
					int maxIndex = 0;
					if (maxSamples >= vk::SampleCountFlagBits::e64) maxIndex = 6;
					else if (maxSamples >= vk::SampleCountFlagBits::e32) maxIndex = 5;
					else if (maxSamples >= vk::SampleCountFlagBits::e16) maxIndex = 4;
					else if (maxSamples >= vk::SampleCountFlagBits::e8) maxIndex = 3;
					else if (maxSamples >= vk::SampleCountFlagBits::e4) maxIndex = 2;
					else if (maxSamples >= vk::SampleCountFlagBits::e2) maxIndex = 1;

					const char* allItems[] = { "Off (1x)", "2x", "4x", "8x", "16x", "32x", "64x" };
					
					int item_current = 0;
					switch(settings->msaaSamples) {
						case vk::SampleCountFlagBits::e1: item_current = 0; break;
						case vk::SampleCountFlagBits::e2: item_current = 1; break;
						case vk::SampleCountFlagBits::e4: item_current = 2; break;
						case vk::SampleCountFlagBits::e8: item_current = 3; break;
						case vk::SampleCountFlagBits::e16: item_current = 4; break;
						case vk::SampleCountFlagBits::e32: item_current = 5; break;
						case vk::SampleCountFlagBits::e64: item_current = 6; break;
						default: break;
					}
					
					// Clamp current item to max supported just in case
					if (item_current > maxIndex) 
					{
						item_current = maxIndex;
						// Force update the setting itself so the user doesn't crash on boot if they somehow saved a bad setting
						switch(maxIndex) {
							case 0: settings->msaaSamples = vk::SampleCountFlagBits::e1; break;
							case 1: settings->msaaSamples = vk::SampleCountFlagBits::e2; break;
							case 2: settings->msaaSamples = vk::SampleCountFlagBits::e4; break;
							case 3: settings->msaaSamples = vk::SampleCountFlagBits::e8; break;
							case 4: settings->msaaSamples = vk::SampleCountFlagBits::e16; break;
							case 5: settings->msaaSamples = vk::SampleCountFlagBits::e32; break;
							case 6: settings->msaaSamples = vk::SampleCountFlagBits::e64; break;
						}
					}

					if (ImGui::Combo("MSAA", &item_current, allItems, maxIndex + 1))
					{
						switch(item_current) {
							case 0: settings->msaaSamples = vk::SampleCountFlagBits::e1; break;
							case 1: settings->msaaSamples = vk::SampleCountFlagBits::e2; break;
							case 2: settings->msaaSamples = vk::SampleCountFlagBits::e4; break;
							case 3: settings->msaaSamples = vk::SampleCountFlagBits::e8; break;
							case 4: settings->msaaSamples = vk::SampleCountFlagBits::e16; break;
							case 5: settings->msaaSamples = vk::SampleCountFlagBits::e32; break;
							case 6: settings->msaaSamples = vk::SampleCountFlagBits::e64; break;
						}
					}
				}
			}
		}

		// Point Light Component
		if (auto* settings = gm.getComponent<PointLightComponent>(selectedEntity))
		{
			if (ImGui::CollapsingHeader("Point Light Component", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::ColorEdit3("Color", glm::value_ptr(settings->color));
				ImGui::DragFloat("Intensity", &settings->intensity, 1.0f, 0.0f, 1000.0f);
				ImGui::DragFloat("Radius", &settings->radius, 0.1f, 0.1f, 100.0f);
				ImGui::DragFloat3("Direction", glm::value_ptr(settings->direction), 0.1f);
				ImGui::DragFloat("Inner Cone Angle", &settings->innerConeAngle, 0.1f, 0.0f, 180.0f);
				ImGui::DragFloat("Outer Cone Angle", &settings->outerConeAngle, 0.1f, 0.0f, 180.0f);
				const char* typeItems[] = { "Point Light", "Spot Light" };
				int typeCurrent = settings->type;
				if (ImGui::Combo("Type", &typeCurrent, typeItems, IM_ARRAYSIZE(typeItems)))
				{
					settings->type = typeCurrent;
				}
			}
		}

		if (auto* globalIllumination = gm.getComponent<LightProbeGridComponent>(selectedEntity))
		{
			if (ImGui::CollapsingHeader("Global Illumination Component", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (ImGui::Button("Bake Global Illumination", ImVec2(200, 20)))
				{
					globalIllumination->needBake = true;
				}
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
